#include "k_bucket.hpp"

//BEGIN contact
k_bucket::contact::contact(
	const net::endpoint & endpoint_in,
	const std::string & remote_ID_in
):
	endpoint(endpoint_in),
	remote_ID(remote_ID_in),
	last_seen(std::time(NULL)),
	ping_sent(false)
{
	assert(!remote_ID.empty());
	//contact starts timed out
	last_seen -= protocol_udp::contact_timeout;
}

bool k_bucket::contact::active_ping()
{
	if(!ping_sent && std::time(NULL) - last_seen >
		protocol_udp::contact_timeout - protocol_udp::response_timeout)
	{
		/*
		We may get to pinging a contact long after it times out if multiple
		contacts get bunched up. We set the last_seen time such that we give the
		ping <ping_timeout> seconds to timeout.
		*/
		last_seen = std::time(NULL) - (protocol_udp::contact_timeout - protocol_udp::response_timeout);
		ping_sent = true;
		return true;
	}
	return false;
}

bool k_bucket::contact::reserve_ping()
{
	//we don't care how long a contact has been in reserve
	if(!ping_sent){
		/*
		Pretend to be idle for <ping_timeout seconds> less than protocol_udp::timeout
		so we can use the same function to check for active and reserve timeouts.
		*/
		last_seen = std::time(NULL) - (protocol_udp::contact_timeout - protocol_udp::response_timeout);
		ping_sent = true;
		return true;
	}
	return false;
}

bool k_bucket::contact::timeout()
{
	/*
	Do not time out connection until we've sent a ping. If multiple pings come
	at the same time some may be delayed past the normal timeout to spread out
	pings.
	*/
	return ping_sent && std::time(NULL) - last_seen > protocol_udp::response_timeout;
}

void k_bucket::contact::touch()
{
	last_seen = std::time(NULL);
	ping_sent = false;
}
//END contact

k_bucket::k_bucket(atomic_int<unsigned> & active_cnt_in):
	active_cnt(active_cnt_in)
{

}

void k_bucket::add_reserve(const net::endpoint & endpoint, const std::string remote_ID)
{
	//if node active then touch it
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->remote_ID == remote_ID && (*it_cur)->endpoint == endpoint){
			(*it_cur)->touch();
			return;
		}else if((*it_cur)->endpoint == endpoint){
			LOG << "ID change " << (*it_cur)->remote_ID << " " << remote_ID;
			return;
		}
	}

	//node not active, add to reserve if not already in reserve
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin(),
		it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->endpoint == endpoint){
			//endpoint already exists in reserve
			return;
		}
	}
	LOG << "reserve: " << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Bucket_Reserve.push_back(boost::shared_ptr<contact>(new contact(endpoint, remote_ID)));
}

bool k_bucket::exists_active(const net::endpoint & ep)
{
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
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
	/*
	Remove active contacts that timed out. Note that contacts which haven't been
	pinged are not allowed to time out. Once a contact is pinged it's timer is
	reset so it has adaquate time to wait for a response. This natually spreads
	out pings.
	*/
	for(std::list<boost::shared_ptr<contact> >::iterator
		it_cur = Bucket_Active.begin(); it_cur != Bucket_Active.end();)
	{
		if((*it_cur)->timeout()){
			LOG << "timed out: " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
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
			return boost::optional<net::endpoint>((*it_cur)->endpoint);
		}
	}

	//if there is space ping reserve to try to move from reserve to active
	unsigned needed = protocol_udp::bucket_size - Bucket_Active.size();
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin();
		it_cur != Bucket_Reserve.end() && needed; --needed)
	{
		if((*it_cur)->reserve_ping()){
			return boost::optional<net::endpoint>((*it_cur)->endpoint);
		}else{
			//reserve contact won't timeout unless it was pinged
			if((*it_cur)->timeout()){
				LOG << "timed out: " << (*it_cur)->endpoint.IP() << " " << (*it_cur)->endpoint.port();
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
		if((*it_cur)->remote_ID == remote_ID && (*it_cur)->endpoint == from){
			(*it_cur)->touch();
			return;
		}else if((*it_cur)->endpoint == from){
			LOG << "ID change " << (*it_cur)->remote_ID << " " << remote_ID;
			return;
		}
	}

	//if node reserved touch it, and move it to active if there's space
	for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin(),
		it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
	{
		//touch if remote_ID not known, or if it's known and didn't change
		if(((*it_cur)->remote_ID.empty() || (*it_cur)->remote_ID == remote_ID)
			&& (*it_cur)->endpoint == from)
		{
			if(Bucket_Active.size() < protocol_udp::bucket_size){
				LOG << "reserve -> active: " << (*it_cur)->endpoint.IP() << " "
					<< (*it_cur)->endpoint.port();
				++active_cnt;
				(*it_cur)->touch();
				Bucket_Active.push_front(*it_cur);
				Bucket_Reserve.erase(it_cur);
			}
			return;
		}
	}

	/*
	Node not active or in reserve. This is not a pong, but another type of
	response which counts as a pong.
	*/
	if(Bucket_Active.size() < protocol_udp::bucket_size){
		//add to routing table
		LOG << "active: " << from.IP() << " " << from.port() << " " << remote_ID;
		++active_cnt;
		Bucket_Active.push_front(boost::shared_ptr<contact>(new contact(from, remote_ID)));
	}else{
		//add to reserve if not already in reserve
		for(std::list<boost::shared_ptr<contact> >::iterator it_cur = Bucket_Reserve.begin(),
			it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
		{
			if((*it_cur)->endpoint == from){
				//endpoint already exists in reserve
				return;
			}
		}
		LOG << "reserve: " << from.IP() << " " << from.port() << " " << remote_ID;
		Bucket_Reserve.push_back(boost::shared_ptr<contact>(new contact(from, remote_ID)));
	}
}
