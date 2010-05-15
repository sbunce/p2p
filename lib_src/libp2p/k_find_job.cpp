#include "k_find_job.hpp"

//BEGIN contact
k_find_job::contact::contact(
	const net::endpoint & endpoint_in,
	const unsigned delay
):
	endpoint(endpoint_in),
	time(std::time(NULL) + delay),
	sent(false),
	timeout_cnt(0)
{

}

bool k_find_job::contact::send()
{
	if(!sent && std::time(NULL) > time){
		time = std::time(NULL) + protocol_udp::response_timeout;
		sent = true;
		return true;
	}
	return false;
}

bool k_find_job::contact::timeout()
{
	if(sent && std::time(NULL) > time){
		time = std::time(NULL);
		sent = false;
		++timeout_cnt;
		return true;
	}
	return false;
}

unsigned k_find_job::contact::timeout_count()
{
	return timeout_cnt;
}
//END contact

k_find_job::k_find_job(
	const boost::function<void (const net::endpoint &)> & call_back_in
):
	call_back(call_back_in)
{
	assert(call_back);
}

void k_find_job::add(const mpa::mpint & dist, const net::endpoint & ep)
{
	if(Memoize.find(ep) == Memoize.end()){
		Memoize.insert(ep);
		Store.insert(std::make_pair(dist, boost::shared_ptr<contact>(new contact(ep))));
	}
}

void k_find_job::add_initial(const std::multimap<mpa::mpint, net::endpoint> & hosts)
{
	assert(Memoize.empty());
	unsigned delay = 0;
	int no_delay_cnt = 16;
	for(std::multimap<mpa::mpint, net::endpoint>::const_iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		if(Memoize.find(it_cur->second) == Memoize.end()){
			Memoize.insert(it_cur->second);
			Store.insert(std::make_pair(it_cur->first,
				boost::shared_ptr<contact>(new contact(it_cur->second, no_delay_cnt-- > 0 ? 0 : delay++))));
		}
	}
}

std::list<net::endpoint> k_find_job::find_node()
{
	//check timeouts
	std::list<boost::shared_ptr<contact> > timeout;
	for(std::multimap<mpa::mpint, boost::shared_ptr<contact> >::iterator
		it_cur = Store.begin(); it_cur != Store.end();)
	{
		if(it_cur->second->timeout()
			&& it_cur->second->timeout_count() > protocol_udp::retransmit_limit){
			timeout.push_back(it_cur->second);
			Store.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
	if(!timeout.empty()){
		//insert timed out contacts at end by setting distance to maximum
		mpa::mpint max(std::string(SHA1::hex_size, 'F'), 16);
		for(std::list<boost::shared_ptr<contact> >::iterator it_cur = timeout.begin(),
			it_end = timeout.end(); it_cur != it_end; ++it_cur)
		{
			LOG << "retransmit limit reached " << (*it_cur)->endpoint.IP()
				<< " " << (*it_cur)->endpoint.port();
			Store.insert(std::make_pair(max, *it_cur));
		}
	}
	//check for endpoint to send find_node to
	std::list<net::endpoint> jobs;
	for(std::multimap<mpa::mpint, boost::shared_ptr<contact> >::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->send()){
			LOG << it_cur->second->endpoint.IP() << " " << it_cur->second->endpoint.port();
			jobs.push_back(it_cur->second->endpoint);
		}
	}
	return jobs;
}

void k_find_job::recv_host_list(const net::endpoint & from,
	const std::list<net::endpoint> & hosts, const mpa::mpint & dist)
{
	if(dist == "0"){
		//remote host claims to be node we're looking for
		call_back(from);
	}
	//erase endpoint that sent host_list and add it to found collection
	for(std::multimap<mpa::mpint, boost::shared_ptr<contact> >::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->endpoint == from){
			Found.insert(std::make_pair(dist, from));
			while(Found.size() > protocol_udp::max_store){
				std::multimap<mpa::mpint, net::endpoint>::iterator iter = Found.end();
				--iter;
				Found.erase(iter);
			}
			Store.erase(it_cur);
			break;
		}
	}
	/*
	Add endpoints that remote host sent. We don't know the distance of the
	endpoints to the ID to find so we assume they're one closer.
	*/
	mpa::mpint new_dist = (dist == "0" ? "0" : dist - "1");
	for(std::list<net::endpoint>::const_iterator it_cur = hosts.begin(),
		it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		add(new_dist, *it_cur);
	}
}
