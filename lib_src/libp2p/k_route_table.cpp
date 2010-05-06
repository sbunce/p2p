#include "k_route_table.hpp"

k_route_table::k_route_table():
	local_ID(db::table::prefs::get_ID())
{

}

void k_route_table::add_reserve(const std::string & remote_ID,
	const net::endpoint & endpoint)
{
	unsigned bucket_num = k_func::bucket_num(local_ID, remote_ID);
	if(endpoint.version() == net::IPv4){
		Bucket_4[bucket_num].add_reserve(remote_ID, endpoint);
	}else{
		Bucket_6[bucket_num].add_reserve(remote_ID, endpoint);
	}
}

std::list<std::pair<net::endpoint, unsigned char> > k_route_table::find_node(
	const std::string & remote_ID, const std::string & ID_to_find)
{
	//get nodes which are closer
	std::map<mpa::mpint, std::pair<std::string, net::endpoint> > hosts_4;
	std::map<mpa::mpint, std::pair<std::string, net::endpoint> > hosts_6;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x].find_node(ID_to_find, hosts_4);
		Bucket_6[x].find_node(ID_to_find, hosts_6);
	}

	//combine IPv4 and IPv6
	std::map<mpa::mpint, std::pair<std::string, net::endpoint> > hosts;
	hosts.insert(hosts_4.begin(), hosts_4.end());
	hosts.insert(hosts_6.begin(), hosts_6.end());

	//calculate buckets the remote host should expect to put the hosts in
	std::list<std::pair<net::endpoint, unsigned char> > hosts_final;
	for(std::map<mpa::mpint, std::pair<std::string, net::endpoint> >::iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		hosts_final.push_back(std::make_pair(it_cur->second.second,
			k_func::bucket_num(remote_ID, it_cur->second.first)));
	}
	return hosts_final;
}

boost::optional<net::endpoint> k_route_table::ping()
{
	boost::optional<net::endpoint> endpoint;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		endpoint = Bucket_4[x].ping();
		if(endpoint){
			return endpoint;
		}
		endpoint = Bucket_6[x].ping();
		if(endpoint){
			return endpoint;
		}
	}
	return boost::optional<net::endpoint>();
}

void k_route_table::pong(const std::string & remote_ID,
	const net::endpoint & endpoint)
{
	unsigned bucket_num = k_func::bucket_num(local_ID, remote_ID);
	if(endpoint.version() == net::IPv4){
		Bucket_4[bucket_num].pong(remote_ID, endpoint);
	}else{
		Bucket_6[bucket_num].pong(remote_ID, endpoint);
	}
}
