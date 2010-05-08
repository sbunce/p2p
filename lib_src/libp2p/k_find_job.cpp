#include "k_find_job.hpp"

//BEGIN contact

k_find_job::contact::contact(const net::endpoint & endpoint_in):
	endpoint(endpoint_in),
	find_node_sent(false),
	find_node_cnt(0)
{

}

k_find_job::contact::contact(const contact & C):
	endpoint(C.endpoint),
	time_sent(C.time_sent),
	find_node_sent(C.find_node_sent),
	find_node_cnt(C.find_node_cnt)
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
	const boost::function<void (const net::endpoint)> & call_back_in
):
	ID_to_find(ID_to_find_in),
	call_back(call_back_in)
{

}

boost::optional<net::endpoint> k_find_job::find_node()
{
	for(std::multimap<mpa::mpint, contact>::iterator it_cur = Store.begin(),
		it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second.send()){
			return boost::optional<net::endpoint>(it_cur->second.endpoint);
		}
	}
	return boost::optional<net::endpoint>();
}

void k_find_job::recv_host_list(const net::endpoint & from,
	const std::list<net::endpoint> & hosts)
{
	for(std::multimap<mpa::mpint, contact>::iterator it_cur = Store.begin(),
		it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second.endpoint == from){
			assert(it_cur->first > "0");
			mpa::mpint new_dist = it_cur->first - "1";
			for(std::list<net::endpoint>::const_iterator h_it_cur = hosts.begin(),
				h_it_end = hosts.end(); h_it_cur != h_it_end; ++h_it_cur)
			{
				Store.insert(std::make_pair(new_dist, contact(*h_it_cur)));
			}
			break;
		}
	}
}
