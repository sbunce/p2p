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
		Store.insert(std::make_pair(it_cur->first,
			boost::shared_ptr<contact>(new contact(it_cur->second))));
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

void k_find_job::recv_host_list(const net::endpoint & from,
	const std::list<net::endpoint> & hosts)
{

	//add endpoints to set to look for
	for(std::multimap<mpa::mpint, boost::shared_ptr<contact> >::iterator
		it_cur = Store.begin(), it_end = Store.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->endpoint == from){
			/*
			Distance of nodes in host_list is unknown so we assume it is one less
			than the distance to the node we requested the host_list from.
			*/
			assert(it_cur->first > "0");
			mpa::mpint new_dist = it_cur->first - "1";

			//don't request a host_list from this host again
			Store.erase(it_cur);

			for(std::list<net::endpoint>::const_iterator h_it_cur = hosts.begin(),
				h_it_end = hosts.end(); h_it_cur != h_it_end; ++h_it_cur)
			{
				if(*h_it_cur == from){
					//host says it's the node we're looking for
					call_back(from, ID_to_find);
				}else{
					//add closer nodes
					Store.insert(std::make_pair(new_dist,
						boost::shared_ptr<contact>(new contact(*h_it_cur))));
				}
			}
			break;
		}
	}
}
