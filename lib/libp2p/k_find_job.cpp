#include "k_find_job.hpp"

//BEGIN store_element
k_find_job::store_element::store_element(
	const net::endpoint & endpoint_in,
	const k_contact & contact_in
):
	endpoint(endpoint_in),
	contact(contact_in)
{

}

k_find_job::store_element::store_element(const store_element & SE):
	endpoint(SE.endpoint),
	contact(SE.contact)
{

}
//END store_element

//BEGIN call_back_element
k_find_job::call_back_element::call_back_element(
	const boost::function<void (const net::endpoint &)> call_back_in,
	const call_back_t type_in
):
	call_back(call_back_in),
	type(type_in)
{

}

k_find_job::call_back_element::call_back_element(const call_back_element & CBE):
	call_back(CBE.call_back),
	type(CBE.type)
{

}
//END call_back_element

k_find_job::k_find_job(const std::multimap<mpa::mpint, net::endpoint> & hosts):
	time(std::time(NULL) + protocol_udp::find_timeout)
{
	unsigned delay = 0;
	int no_delay_cnt = protocol_udp::no_delay_count;
	for(std::multimap<mpa::mpint, net::endpoint>::const_iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		Memoize.insert(it_cur->second);
		Store.insert(std::make_pair(it_cur->first,
			store_element(it_cur->second, k_contact(0, no_delay_cnt-- > 0 ? 0 : delay++))));
	}
}

void k_find_job::add(const net::endpoint & ep, const mpa::mpint & dist)
{
	if(Memoize.find(ep) == Memoize.end()){
		Memoize.insert(ep);
		Store.insert(std::make_pair(dist, store_element(ep, k_contact())));
	}
}

void k_find_job::call_back(const net::endpoint & ep, const mpa::mpint & dist)
{
	for(std::list<call_back_element>::iterator it_cur = Call_Back.begin(),
		it_end = Call_Back.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->type == exact_match){
			if(dist == "0"){
				it_cur->call_back(ep);
			}
		}else{
			it_cur->call_back(ep);
		}
	}
}

std::list<net::endpoint> k_find_job::find_node()
{
	//check timeouts
	std::list<store_element> timeout;
	for(std::multimap<mpa::mpint, store_element>::iterator it_cur = Store.begin();
		it_cur != Store.end();)
	{
		if(it_cur->second.contact.timeout()
			&& it_cur->second.contact.timeout_count() > protocol_udp::retransmit_limit){
			timeout.push_back(it_cur->second);
			Store.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
	if(!timeout.empty()){
		//insert timed out contacts at end by setting distance to maximum
		mpa::mpint max(std::string(SHA1::hex_size, 'F'), 16);
		for(std::list<store_element>::iterator it_cur = timeout.begin(),
			it_end = timeout.end(); it_cur != it_end; ++it_cur)
		{
			LOG << "retransmit limit reached " << it_cur->endpoint.IP()
				<< " " << it_cur->endpoint.port();
			Store.insert(std::make_pair(max, *it_cur));
		}
	}
	//check for endpoint to send find_node to
	std::list<net::endpoint> jobs;
	for(std::multimap<mpa::mpint, store_element>::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second.contact.send()){
			//LOG << it_cur->second.endpoint.IP() << " " << it_cur->second.endpoint.port();
			jobs.push_back(it_cur->second.endpoint);
		}
	}
	return jobs;
}

const std::multimap<mpa::mpint, net::endpoint> & k_find_job::found()
{
	return Found;
}

void k_find_job::recv_host_list(const net::endpoint & from,
	const std::list<net::endpoint> & hosts, const mpa::mpint & dist)
{
	call_back(from, dist);

	//erase endpoint that sent host_list and add it to found collection
	for(std::multimap<mpa::mpint, store_element>::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second.endpoint == from){
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
		add(*it_cur, new_dist);
	}
}

void k_find_job::register_call_back(
	const boost::function<void (const net::endpoint &)> & call_back,
	const call_back_t type)
{
	Call_Back.push_back(call_back_element(call_back, type));
	if(type == any_will_do){
		for(std::multimap<mpa::mpint, net::endpoint>::iterator it_cur = Found.begin(),
			it_end = Found.end(); it_cur != it_end; ++it_cur)
		{
			call_back(it_cur->second);
		}
	}
}

bool k_find_job::timeout()
{
	return std::time(NULL) > time;
}
