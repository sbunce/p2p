#include "k_bucket.hpp"

//BEGIN contact
k_bucket::contact::contact(
	const std::string & remote_ID_in,
	const network::endpoint & endpoint_in
):
	remote_ID(remote_ID_in),
	endpoint(endpoint_in),
	last_seen(std::time(NULL)),
	ping_sent(false)
{

}

k_bucket::contact::contact(const contact & C):
	endpoint(C.endpoint),
	ping_sent(C.ping_sent),
	last_seen(C.last_seen)
{

}

bool k_bucket::contact::need_ping()
{
	if(!ping_sent && std::time(NULL) - last_seen > protocol_udp::timeout - 60){
		ping_sent = true;
		return true;
	}
	return false;
}

bool k_bucket::contact::timed_out()
{
	return std::time(NULL) - last_seen > protocol_udp::timeout;
}

void k_bucket::contact::touch()
{
	last_seen = std::time(NULL);
	ping_sent = false;
}
//END contact

void k_bucket::erase(const network::endpoint & endpoint)
{
	for(std::list<contact>::iterator iter_cur = Bucket.begin(),
		iter_end = Bucket.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->endpoint == endpoint){
			Bucket.erase(iter_cur);
			return;
		}
	}
}

bool k_bucket::exists(const network::endpoint & endpoint)
{
	for(std::list<contact>::iterator iter_cur = Bucket.begin(),
		iter_end = Bucket.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->endpoint == endpoint){
			return true;
		}
	}
	return false;
}

void k_bucket::find_node(const std::string & ID_to_find,
		std::map<mpa::mpint, std::pair<std::string, network::endpoint> > & hosts)
{
	//calculate distances of all contacts
	for(std::list<contact>::iterator iter_cur = Bucket.begin(),
		iter_end = Bucket.end(); iter_cur != iter_end; ++iter_cur)
	{
		hosts.insert(std::make_pair(k_func::distance(ID_to_find, iter_cur->remote_ID),
			std::make_pair(iter_cur->remote_ID, iter_cur->endpoint)));
	}
	//get rid of all but closest protocol_udp::bucket_size elements
	while(!hosts.empty() && hosts.size() > protocol_udp::host_list_elements){
		std::map<mpa::mpint, std::pair<std::string, network::endpoint> >::iterator
			iter = hosts.end();
			--iter;
		hosts.erase(iter);
	}
}

std::list<network::endpoint> k_bucket::ping()
{
	std::list<network::endpoint> tmp;
	for(std::list<contact>::iterator iter_cur = Bucket.begin(),
		iter_end = Bucket.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->need_ping()){
			tmp.push_back(iter_cur->endpoint);
		}
	}
	return tmp;
}

unsigned k_bucket::size()
{
	return Bucket.size();
}

bool k_bucket::update(const std::string remote_ID,
	const network::endpoint & endpoint)
{
	//try to update
	for(std::list<contact>::iterator iter_cur = Bucket.begin(),
		iter_end = Bucket.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->remote_ID == remote_ID && iter_cur->endpoint == endpoint){
			iter_cur->touch();
			return true;
		}else if(iter_cur->remote_ID != remote_ID && iter_cur->endpoint == endpoint){
			//host changed it's node ID, do not allow update
			return false;
		}
	}

	//try to add
	if(Bucket.size() < protocol_udp::bucket_size){
		Bucket.push_back(contact(remote_ID, endpoint));
		return true;
	}else{
		return false;
	}
}
