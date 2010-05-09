#include "k_route_table.hpp"

k_route_table::k_route_table():
	local_ID(db::table::prefs::get_ID())
{

}

void k_route_table::add_reserve(const net::endpoint & endpoint,
	const std::string & remote_ID)
{
	if(remote_ID.empty()){
		Unknown.push_back(endpoint);
	}else{
		unsigned bucket_num = k_func::bucket_num(local_ID, remote_ID);
		if(endpoint.version() == net::IPv4){
			Bucket_4[bucket_num].add_reserve(endpoint, remote_ID);
		}else{
			Bucket_6[bucket_num].add_reserve(endpoint, remote_ID);
		}
	}
}

std::list<net::endpoint> k_route_table::find_node(const std::string & ID_to_find)
{
	struct func_local{
	static void trim(std::multimap<mpa::mpint, net::endpoint> & hosts)
	{
		while(hosts.size() > protocol_udp::host_list_elements / 2){
			std::multimap<mpa::mpint, net::endpoint>::iterator iter = hosts.end();
			--iter;
			hosts.erase(iter);
		}
	}
	};
	//get nodes which are closer
	std::multimap<mpa::mpint, net::endpoint> hosts_4;
	std::multimap<mpa::mpint, net::endpoint> hosts_6;
	mpa::mpint max_dist = k_func::distance(local_ID, ID_to_find);
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x].find_node(ID_to_find, max_dist, hosts_4);
		func_local::trim(hosts_4);
		Bucket_6[x].find_node(ID_to_find, max_dist, hosts_6);
		func_local::trim(hosts_6);
	}
	//combine IPv4 and IPv6
	std::multimap<mpa::mpint, net::endpoint> hosts;
	hosts.insert(hosts_4.begin(), hosts_4.end());
	hosts.insert(hosts_6.begin(), hosts_6.end());
	std::list<net::endpoint> hosts_final;
	for(std::multimap<mpa::mpint, net::endpoint>::iterator it_cur = hosts.begin(),
		it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		hosts_final.push_back(it_cur->second);
	}
	return hosts_final;
}

std::multimap<mpa::mpint, net::endpoint> k_route_table::find_node_local(
	const std::string & ID_to_find)
{
	//get all nodes
	std::multimap<mpa::mpint, net::endpoint> hosts_4;
	std::multimap<mpa::mpint, net::endpoint> hosts_6;
	std::string max_str(SHA1::hex_size, 'F');
	mpa::mpint max_dist(max_str, 16);
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x].find_node(ID_to_find, max_dist, hosts_4);
		Bucket_6[x].find_node(ID_to_find, max_dist, hosts_6);
	}
	//combine IPv4 and IPv6
	std::multimap<mpa::mpint, net::endpoint> hosts;
	hosts.insert(hosts_4.begin(), hosts_4.end());
	hosts.insert(hosts_6.begin(), hosts_6.end());
	return hosts;
}

boost::optional<net::endpoint> k_route_table::ping()
{
	boost::optional<net::endpoint> ep;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		ep = Bucket_4[x].ping();
		if(ep){
			return boost::optional<net::endpoint>(*ep);
		}
		ep = Bucket_6[x].ping();
		if(ep){
			return boost::optional<net::endpoint>(*ep);
		}
	}
	if(!Unknown.empty()){
		ep = Unknown.front();
		Unknown.pop_front();
		return ep;
	}
	return boost::optional<net::endpoint>();
}

void k_route_table::recv_pong(const net::endpoint & from,
	const std::string & remote_ID)
{
	unsigned bucket_num = k_func::bucket_num(local_ID, remote_ID);
	if(from.version() == net::IPv4){
		Bucket_4[bucket_num].recv_pong(from, remote_ID);
	}else{
		Bucket_6[bucket_num].recv_pong(from, remote_ID);
	}
}
