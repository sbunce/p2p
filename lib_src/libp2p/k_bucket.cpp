#include "k_bucket.hpp"

//BEGIN contact
k_bucket::contact::contact(
	const net::endpoint & endpoint_in,
	const std::string & remote_ID_in
):
	endpoint(endpoint_in),
	remote_ID(remote_ID_in),
	timeout_cnt(0)
{
	touch();
}

bool k_bucket::contact::active_ping()
{
	if(!ping_sent && std::time(NULL) > time){
		time = std::time(NULL) + protocol_udp::contact_timeout;
		ping_sent = true;
		return true;
	}
	return false;
}

bool k_bucket::contact::reserve_ping()
{
	if(!ping_sent){
		time = std::time(NULL) + protocol_udp::response_timeout;
		ping_sent = true;
		return true;
	}
	return false;
}

bool k_bucket::contact::timeout()
{
	if(ping_sent && std::time(NULL) > time){
		touch();
		++timeout_cnt;
		return true;
	}
	return false;
}

unsigned k_bucket::contact::timeout_count()
{
	return timeout_cnt;
}

void k_bucket::contact::touch()
{
	time = std::time(NULL) + protocol_udp::contact_timeout;
	ping_sent = false;
}
//END contact

k_bucket::k_bucket(
	atomic_int<unsigned> & active_cnt_in,
	const boost::function<void (const net::endpoint &, const std::string &)> & route_table_call_back_in
):
	active_cnt(active_cnt_in),
	route_table_call_back(route_table_call_back_in)
{

}

void k_bucket::add_reserve(const net::endpoint & ep, const std::string remote_ID)
{
	if(exists(ep)){
		return;
	}
	LOG << "reserve: " << ep.IP() << " " << ep.port() << " " << remote_ID;
	Bucket_Reserve.push_back(boost::shared_ptr<contact>(new contact(ep, remote_ID)));
}

bool k_bucket::exists(const net::endpoint & ep)
{
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->endpoint == ep){
			return true;
		}
	}
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin(),
		it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->endpoint == ep){
			return true;
		}
	}
	return false;
}

void k_bucket::find_node(const std::string & ID_to_find,
	std::multimap<mpa::mpint, net::endpoint> & hosts)
{
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		mpa::mpint dist = k_func::distance(ID_to_find, (*it_cur)->remote_ID);
		hosts.insert(std::make_pair(k_func::distance(ID_to_find, (*it_cur)->remote_ID),
			(*it_cur)->endpoint));
	}
}

void k_bucket::find_node(const net::endpoint & from, const std::string & ID_to_find,
	std::multimap<mpa::mpint, net::endpoint> & hosts)
{
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->endpoint != from){
			mpa::mpint dist = k_func::distance(ID_to_find, (*it_cur)->remote_ID);
			hosts.insert(std::make_pair(k_func::distance(ID_to_find, (*it_cur)->remote_ID),
				(*it_cur)->endpoint));
		}
	}
}

boost::optional<net::endpoint> k_bucket::ping()
{
	//removed active contacts which timed out
	for(std::list<boost::shared_ptr<contact> >::iterator
		it_cur = Bucket_Active.begin(); it_cur != Bucket_Active.end();)
	{
		if((*it_cur)->timeout()
			&& (*it_cur)->timeout_count() > protocol_udp::retransmit_limit)
		{
			LOG << "timeout: " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
			--active_cnt;
			it_cur = Bucket_Active.erase(it_cur);
		}else{
			++it_cur;
		}
	}
	//check if active node needs ping
	boost::optional<net::endpoint> ep;
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->active_ping()){
			LOG << "active: " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
			return (*it_cur)->endpoint;
		}
	}
	//if there is space ping reserve to try to move from reserve to active
	unsigned needed = protocol_udp::bucket_size - Bucket_Active.size();
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin();
		it_cur != Bucket_Reserve.end() && needed; --needed)
	{
		if((*it_cur)->reserve_ping()){
			LOG << "reserve: " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
			return (*it_cur)->endpoint;
		}else{
			//reserve contact won't timeout unless it was pinged
			if((*it_cur)->timeout()
				&& (*it_cur)->timeout_count() > protocol_udp::retransmit_limit)
			{
				LOG << "timeout: " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
				it_cur = Bucket_Reserve.erase(it_cur);
			}else{
				++it_cur;
			}
		}
	}
	return boost::optional<net::endpoint>();
}

void k_bucket::recv_pong(const net::endpoint & from, const std::string & remote_ID)
{
	//if node active then touch it
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->endpoint == from && (*it_cur)->remote_ID == remote_ID){
			LOG << "touch " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
			(*it_cur)->touch();
			return;
		}else if((*it_cur)->endpoint == from){
			LOG << "ID change " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
			(*it_cur)->touch();
			Bucket_Reserve.push_back(*it_cur);
			Bucket_Active.erase(it_cur);
			return;
		}
	}
	//if node reserved touch it, and move it to active if there's space
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin(),
		it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->endpoint == from && (*it_cur)->remote_ID == remote_ID){
			if(Bucket_Active.size() < protocol_udp::bucket_size){
				LOG << "reserve -> active: " << (*it_cur)->endpoint.IP() << " "
					<< (*it_cur)->endpoint.port();
				++active_cnt;
				(*it_cur)->touch();
				Bucket_Active.push_front(*it_cur);
				Bucket_Reserve.erase(it_cur);
				route_table_call_back(from, remote_ID);
			}
			return;
		}else if((*it_cur)->endpoint == from){
			LOG << "ID change " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
			Bucket_Reserve.push_back(*it_cur);
			Bucket_Reserve.erase(it_cur);
			return;
		}
	}
	//not in active or reserve, response message that counts as pong
	if(Bucket_Active.size() < protocol_udp::bucket_size){
		//add to routing table
		LOG << "active: " << from.IP() << " " << from.port() << " " << remote_ID;
		++active_cnt;
		Bucket_Active.push_front(boost::shared_ptr<contact>(new contact(from, remote_ID)));
		route_table_call_back(from, remote_ID);
		return;
	}else{
		//add to reserve
		LOG << "reserve: " << from.IP() << " " << from.port() << " " << remote_ID;
		Bucket_Reserve.push_back(boost::shared_ptr<contact>(new contact(from, remote_ID)));
		return;
	}
}
