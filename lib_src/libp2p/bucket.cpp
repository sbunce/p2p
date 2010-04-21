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
		/*
		We may get to pinging a contact long after it times out if multiple
		contacts get bunched up. We set the last_seen time such that we give the
		ping <ping_timeout> seconds to timeout.
		*/
		last_seen = std::time(NULL) - (protocol_udp::timeout - ping_timeout);
		ping_sent = true;
		return true;
	}
	return false;
}

bool bucket::contact::reserve_ping()
{
	//we don't care how long a contact has been in reserve
	if(!ping_sent){
		/*
		Pretend to be idle for <ping_timeout seconds> less than protocol_udp::timeout
		so we can use the same function to check for active and reserve timeouts.
		*/
		last_seen = std::time(NULL) - (protocol_udp::timeout - ping_timeout);
		ping_sent = true;
		return true;
	}
	return false;
}

bool bucket::contact::timed_out()
{
	/*
	Do not time out connection until we've sent a ping. If multiple pings come
	at the same time some may be delayed past the normal timeout to spread out
	pings.
	*/
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
	for(std::list<contact>::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->remote_ID == remote_ID && it_cur->endpoint == endpoint){
			it_cur->touch();
			LOG << "touch: " << it_cur->endpoint.IP() << " " << it_cur->endpoint.port()
				<< it_cur->remote_ID;
			return;
		}else if(it_cur->endpoint == endpoint){
			LOG << "ID change " << it_cur->remote_ID << " " << remote_ID;
			return;
		}
	}

	//node not active, add to reserve if not already in reserve
	for(std::list<contact>::iterator it_cur = Bucket_Reserve.begin(),
		it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->endpoint == endpoint){
			//endpoint already exists in reserve
			return;
		}
	}
	LOG << "reserve: " << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Bucket_Reserve.push_back(contact(remote_ID, endpoint));
}

void bucket::find_node(const std::string & ID_to_find,
	std::map<mpa::mpint, std::pair<std::string, network::endpoint> > & hosts)
{
	//calculate distances of all contacts
	for(std::list<contact>::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		hosts.insert(std::make_pair(kad_func::distance(ID_to_find, it_cur->remote_ID),
			std::make_pair(it_cur->remote_ID, it_cur->endpoint)));
	}

	//get rid of all but closest elements
	while(!hosts.empty() && hosts.size() > protocol_udp::host_list_elements){
		std::map<mpa::mpint, std::pair<std::string, network::endpoint> >::iterator
			iter = hosts.end();
			--iter;
		hosts.erase(iter);
	}
}

boost::optional<network::endpoint> bucket::ping()
{
	//remove active nodes that timed out
	for(std::list<contact>::iterator it_cur = Bucket_Active.begin();
		it_cur != Bucket_Active.end();)
	{
		if(it_cur->timed_out()){
			LOG << "timed out: " << it_cur->endpoint.IP() << " " << it_cur->endpoint.port();
			it_cur = Bucket_Active.erase(it_cur);
		}else{
			++it_cur;
		}
	}

	//ping active nodes
	for(std::list<contact>::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->active_ping()){
			return boost::optional<network::endpoint>(it_cur->endpoint);
		}
	}

	//ping reserve to try to move from reserve to active if there is space
	unsigned needed = protocol_udp::bucket_size - Bucket_Active.size();
	for(std::list<contact>::iterator it_cur = Bucket_Reserve.begin();
		it_cur != Bucket_Reserve.end() && needed; --needed)
	{
		if(it_cur->reserve_ping()){
			return boost::optional<network::endpoint>(it_cur->endpoint);
		}else{
			//reserve contact won't timeout unless it was pinged
			if(it_cur->timed_out()){
				LOG << "timed out: " << it_cur->endpoint.IP() << " " << it_cur->endpoint.port();
				it_cur = Bucket_Reserve.erase(it_cur);
			}else{
				++it_cur;
			}
		}
	}

	return boost::optional<network::endpoint>();
}

void bucket::pong(const std::string & remote_ID,
	const network::endpoint & endpoint)
{
	//if node active then touch it
	for(std::list<contact>::iterator it_cur = Bucket_Active.begin(),
		it_end = Bucket_Active.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->remote_ID == remote_ID && it_cur->endpoint == endpoint){
			it_cur->touch();
			LOG << "touch: " << it_cur->endpoint.IP() << " " << it_cur->endpoint.port();
			return;
		}else if(it_cur->endpoint == endpoint){
			LOG << "ID change " << it_cur->remote_ID << " " << remote_ID;
			return;
		}
	}

	//if node reserved touch it, and move it to active if there's space
	for(std::list<contact>::iterator it_cur = Bucket_Reserve.begin(),
		it_end = Bucket_Reserve.end(); it_cur != it_end; ++it_cur)
	{
		//touch if remote_ID not known, or if it's known and didn't change
		if((it_cur->remote_ID.empty() || it_cur->remote_ID == remote_ID)
			&& it_cur->endpoint == endpoint)
		{
			if(Bucket_Active.size() < protocol_udp::bucket_size){
				LOG << "reserve -> active: " << it_cur->endpoint.IP() << " "
					<< it_cur->endpoint.port();
				it_cur->remote_ID = remote_ID;
				it_cur->touch();
				Bucket_Active.push_front(*it_cur);
				Bucket_Reserve.erase(it_cur);
			}
			return;
		}
	}
}
