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
//END contact

k_bucket::k_bucket()
{

}

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

std::vector<network::endpoint> k_bucket::ping()
{
	std::vector<network::endpoint> vec;
	for(std::list<contact>::iterator iter_cur = Bucket.begin(),
		iter_end = Bucket.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->need_ping()){
			vec.push_back(iter_cur->endpoint);
		}
	}
	return vec;
}

unsigned k_bucket::size()
{
	return Bucket.size();
}

bool k_bucket::update(const std::string remote_ID,
	const network::endpoint & endpoint)
{
	//erase if exists, and re-add
	erase(endpoint);
	if(Bucket.size() < protocol_udp::bucket_size){
		Bucket.push_back(contact(remote_ID, endpoint));
		return true;
	}
	return false;
}
