#include "bucket.hpp"

//BEGIN contact
bucket::contact::contact(
	const std::string & remote_ID_in,
	const network::endpoint & endpoint_in
):
	remote_ID(remote_ID_in),
	endpoint(endpoint_in),
	last_seen(std::time(NULL)),
	ping_sent(false)
{
	//contact starts timed out
	last_seen -= protocol_udp::timeout;
}

bucket::contact::contact(const contact & C):
	remote_ID(C.remote_ID),
	endpoint(C.endpoint),
	ping_sent(C.ping_sent),
	last_seen(C.last_seen)
{

}

bool bucket::contact::active_ping()
{
	if(!ping_sent && std::time(NULL) - last_seen > protocol_udp::timeout - ping_timeout){
		ping_sent = true;
		return true;
	}
	return false;
}

bool bucket::contact::reserve_ping()
{
	if(!ping_sent){
		ping_sent = true;
		last_seen = std::time(NULL) - (protocol_udp::timeout - ping_timeout);
		return true;
	}
	return false;
}

bool bucket::contact::timed_out()
{
	return ping_sent && std::time(NULL) - last_seen > ping_timeout;
}

void bucket::contact::touch()
{
	last_seen = std::time(NULL);
	ping_sent = false;
}
//END contact

void bucket::add_reserve(const std::string remote_ID,
	const network::endpoint & endpoint)
{
	//if node active then touch it
	for(std::list<contact>::iterator iter_cur = Bucket_Active.begin(),
		iter_end = Bucket_Active.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->remote_ID == remote_ID && iter_cur->endpoint == endpoint){
			iter_cur->touch();
			LOG << "touch: " << iter_cur->endpoint.IP() << " " << iter_cur->endpoint.port()
				<< iter_cur->remote_ID;
			return;
		}else if(iter_cur->endpoint == endpoint){
			LOG << "node ID changed, from " << iter_cur->remote_ID << " to " << remote_ID;
			return;
		}
	}

	//node not active, add to reserve if not already in reserve
	for(std::list<contact>::iterator iter_cur = Bucket_Reserve.begin(),
		iter_end = Bucket_Reserve.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->endpoint == endpoint){
			//endpoint already exists in reserve
			return;
		}
	}
	LOG << "add reserve: " << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Bucket_Reserve.push_back(contact(remote_ID, endpoint));
}

void bucket::find_node(const std::string & ID_to_find,
	std::map<mpa::mpint, std::pair<std::string, network::endpoint> > & hosts)
{
	//calculate distances of all contacts
	for(std::list<contact>::iterator iter_cur = Bucket_Active.begin(),
		iter_end = Bucket_Active.end(); iter_cur != iter_end; ++iter_cur)
	{
		hosts.insert(std::make_pair(kad_func::distance(ID_to_find, iter_cur->remote_ID),
			std::make_pair(iter_cur->remote_ID, iter_cur->endpoint)));
	}

	//get rid of all but closest elements
	while(!hosts.empty() && hosts.size() > protocol_udp::host_list_elements){
		std::map<mpa::mpint, std::pair<std::string, network::endpoint> >::iterator
			iter = hosts.end();
			--iter;
		hosts.erase(iter);
	}
}

void bucket::ping(std::set<network::endpoint> & hosts)
{
	//remove active nodes that timed out
	for(std::list<contact>::iterator iter_cur = Bucket_Active.begin();
		iter_cur != Bucket_Active.end();)
	{
		if(iter_cur->timed_out()){
			LOG << "timed out: " << iter_cur->endpoint.IP() << " " << iter_cur->endpoint.port();
			iter_cur = Bucket_Active.erase(iter_cur);
		}else{
			++iter_cur;
		}
	}

	//ping active nodes
	for(std::list<contact>::iterator iter_cur = Bucket_Active.begin(),
		iter_end = Bucket_Active.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->active_ping()){
			hosts.insert(iter_cur->endpoint);
		}
	}

	//ping reserve to try to move from reserve to active if there is space
	unsigned needed = protocol_udp::bucket_size - Bucket_Active.size();
	for(std::list<contact>::iterator iter_cur = Bucket_Reserve.begin();
		iter_cur != Bucket_Reserve.end() && needed; --needed)
	{
		if(iter_cur->reserve_ping()){
			hosts.insert(iter_cur->endpoint);
			++iter_cur;
		}else{
			if(iter_cur->timed_out()){
				LOG << "timed out: " << iter_cur->endpoint.IP() << " " << iter_cur->endpoint.port();
				iter_cur = Bucket_Reserve.erase(iter_cur);
			}else{
				++iter_cur;
			}
		}
	}
}

void bucket::pong(const std::string & remote_ID,
	const network::endpoint & endpoint)
{
	//if node active then touch it
	for(std::list<contact>::iterator iter_cur = Bucket_Active.begin(),
		iter_end = Bucket_Active.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->remote_ID == remote_ID && iter_cur->endpoint == endpoint){
			iter_cur->touch();
			LOG << "touch: " << iter_cur->endpoint.IP() << " " << iter_cur->endpoint.port();
			return;
		}else if(iter_cur->endpoint == endpoint){
			LOG << "node ID changed, from " << iter_cur->remote_ID << " to " << remote_ID;
			return;
		}
	}

	//if node reserved touch it, and move it to active if there's space
	for(std::list<contact>::iterator iter_cur = Bucket_Reserve.begin(),
		iter_end = Bucket_Reserve.end(); iter_cur != iter_end; ++iter_cur)
	{
		//touch if remote_ID not known, or if it's known and didn't change
		if((iter_cur->remote_ID.empty() || iter_cur->remote_ID == remote_ID)
			&& iter_cur->endpoint == endpoint)
		{
			if(Bucket_Active.size() < protocol_udp::bucket_size){
				LOG << "reserve -> active: " << iter_cur->endpoint.IP() << " "
					<< iter_cur->endpoint.port();
				iter_cur->remote_ID = remote_ID;
				iter_cur->touch();
				Bucket_Active.push_front(*iter_cur);
				Bucket_Reserve.erase(iter_cur);
			}
			return;
		}
	}
}
