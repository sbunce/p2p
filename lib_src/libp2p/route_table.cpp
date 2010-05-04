#include "route_table.hpp"

route_table::route_table():
	local_ID(db::table::prefs::get_ID())
{

}

void route_table::add_reserve(const std::string & remote_ID,
	const network::endpoint & endpoint)
{
	unsigned bucket_num = kad_func::bucket_num(local_ID, remote_ID);
	Bucket[bucket_num].add_reserve(remote_ID, endpoint);
}

std::list<std::pair<network::endpoint, unsigned char> > route_table::find_node(
	const std::string & remote_ID, const std::string & ID_to_find)
{
	//get nodes which are closer
	std::map<mpa::mpint, std::pair<std::string, network::endpoint> > hosts;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket[x].find_node(ID_to_find, hosts);
	}

	//calculate buckets the remote host should expect to put the hosts in
	std::list<std::pair<network::endpoint, unsigned char> > hosts_final;
	for(std::map<mpa::mpint, std::pair<std::string, network::endpoint> >::iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		hosts_final.push_back(std::make_pair(it_cur->second.second,
			kad_func::bucket_num(remote_ID, it_cur->second.first)));
	}
	return hosts_final;
}

boost::optional<network::endpoint> route_table::ping()
{
	boost::optional<network::endpoint> endpoint;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		endpoint = Bucket[x].ping();
		if(endpoint){
			return endpoint;
		}
	}
	return boost::optional<network::endpoint>();
}

void route_table::pong(const std::string & remote_ID,
	const network::endpoint & endpoint)
{
	unsigned bucket_num = kad_func::bucket_num(local_ID, remote_ID);
	Bucket[bucket_num].pong(remote_ID, endpoint);
}
