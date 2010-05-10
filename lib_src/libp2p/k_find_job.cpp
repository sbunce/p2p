#include "k_find_job.hpp"

//BEGIN contact
k_find_job::contact::contact(const net::endpoint & endpoint_in):
	endpoint(endpoint_in),
	find_node_sent(false),
	find_node_cnt(0)
{

}

bool k_find_job::contact::send()
{
	/*
	Send find_node if one hasn't yet been sent. Or send one if the last find_node
	message timed out. Progressively increase the amount of time between
	find_node messages when timeouts happen.
	*/
	if(!find_node_sent
		|| std::time(NULL) - time_sent > protocol_udp::response_timeout * find_node_cnt)
	{
		find_node_sent = true;
		time_sent = std::time(NULL);
		++find_node_cnt;
		return true;
	}
	return false;
}
//END contact

k_find_job::k_find_job(
	const std::string & ID_to_find_in,
	const boost::function<void (const net::endpoint &, const std::string &)> & call_back_in
):
	ID_to_find(ID_to_find_in),
	call_back(call_back_in)
{
	assert(call_back);
}

void k_find_job::add_local(const std::multimap<mpa::mpint, net::endpoint> & hosts)
{
	for(std::multimap<mpa::mpint, net::endpoint>::const_iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		if(Memoize.find(it_cur->second) == Memoize.end()){
			Memoize.insert(it_cur->second);
			Store.insert(std::make_pair(it_cur->first,
				boost::shared_ptr<contact>(new contact(it_cur->second))));
		}
	}
}

boost::optional<net::endpoint> k_find_job::find_node()
{
	for(std::multimap<mpa::mpint, boost::shared_ptr<contact> >::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->send()){
			return boost::optional<net::endpoint>(it_cur->second->endpoint);
		}
	}
	return boost::optional<net::endpoint>();
}

void k_find_job::recv_host_list(const net::endpoint & from, const std::string & remote_ID,
	const std::list<net::endpoint> & hosts, const std::string & ID_to_find)
{
	//erase endpoint that responded to us
	for(std::multimap<mpa::mpint, boost::shared_ptr<contact> >::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->endpoint == from){
			Store.erase(it_cur);
			break;
		}
	}

	//assume hosts are closer than remote_ID to ID_to_find
	mpa::mpint dist = k_func::distance(remote_ID, ID_to_find);
	mpa::mpint new_dist = (dist == "0" ? "0" : dist - "1");

	//add closer nodes that we haven't seen before
	for(std::list<net::endpoint>::const_iterator it_cur = hosts.begin(),
		it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		if(Memoize.find(*it_cur) == Memoize.end()){
			Memoize.insert(*it_cur);
			Store.insert(std::make_pair(new_dist, boost::shared_ptr<contact>(
				new contact(*it_cur))));
		}
	}
}
