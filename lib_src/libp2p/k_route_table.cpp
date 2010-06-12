#include "k_route_table.hpp"

k_route_table::k_route_table(
	atomic_int<unsigned> & active_cnt,
	const boost::function<void (const net::endpoint &, const std::string &)> & route_table_call_back
):
	local_ID(db::table::prefs::get_ID())
{
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x].reset(new k_bucket(active_cnt, route_table_call_back));
		Bucket_6[x].reset(new k_bucket(active_cnt, route_table_call_back));
	}
}

void k_route_table::add_reserve(const net::endpoint & ep, const std::string & remote_ID)
{
	if(remote_ID.empty()){
		for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
			if(Bucket_4[x]->exists(ep)){
				return;
			}
			if(Bucket_6[x]->exists(ep)){
				return;
			}
		}
		if(Unknown_Reserve.find(ep) != Unknown_Reserve.end()
			|| Unknown_Active.find(ep) != Unknown_Active.end())
		{
			return;
		}
		//LOG << "add unknown: " << ep.IP() << " " << ep.port();
		Unknown_Reserve.insert(ep);
	}else{
		Unknown_Reserve.erase(ep);
		if(Unknown_Active.find(ep) != Unknown_Active.end()){
			return;
		}
		unsigned bucket_num = k_func::bucket_num(local_ID, remote_ID);
		if(ep.version() == net::IPv4){
			Bucket_4[bucket_num]->add_reserve(ep, remote_ID);
		}else{
			Bucket_6[bucket_num]->add_reserve(ep, remote_ID);
		}
	}
}

std::list<net::endpoint> k_route_table::find_node(const net::endpoint & from,
	const std::string & ID_to_find)
{
	struct func_local{
	//trim to host_list size
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
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x]->find_node(from, ID_to_find, hosts_4);
		func_local::trim(hosts_4);
		Bucket_6[x]->find_node(from, ID_to_find, hosts_6);
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
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x]->find_node(ID_to_find, hosts_4);
		Bucket_6[x]->find_node(ID_to_find, hosts_6);
	}
	//combine IPv4 and IPv6
	std::multimap<mpa::mpint, net::endpoint> hosts;
	hosts.insert(hosts_4.begin(), hosts_4.end());
	hosts.insert(hosts_6.begin(), hosts_6.end());
	return hosts;
}

boost::optional<net::endpoint> k_route_table::ping()
{
	//check buckets for endpoint to ping
	boost::optional<net::endpoint> ep;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		ep = Bucket_4[x]->ping();
		if(ep){
			return ep;
		}
		ep = Bucket_6[x]->ping();
		if(ep){
			return ep;
		}
	}
	//check if retransmission needed
	for(std::map<net::endpoint, k_contact>::iterator it_cur = Unknown_Active.begin(),
		it_end = Unknown_Active.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second.send()){
			LOG << "retransmit: " << it_cur->first.IP() << " " << it_cur->first.port();
			return it_cur->first;
		}
	}
	if(!Unknown_Reserve.empty()){
		//ping random endpoint
		boost::uint64_t num = portable::roll(Unknown_Reserve.size());
		std::set<net::endpoint>::iterator it = Unknown_Reserve.begin();
		while(num--){ ++it; }
		ep = *it;
		Unknown_Reserve.erase(it);
		Unknown_Active.insert(std::make_pair(*ep, k_contact()));
		//LOG << "unknown: " << ep->IP() << " " << ep->port();
		return ep;
	}
	return boost::optional<net::endpoint>();
}

void k_route_table::recv_pong(const net::endpoint & from,
	const std::string & remote_ID)
{
	Unknown_Active.erase(from);
	Unknown_Reserve.erase(from);
	unsigned bucket_num = k_func::bucket_num(local_ID, remote_ID);
	if(from.version() == net::IPv4){
		Bucket_4[bucket_num]->recv_pong(from, remote_ID);
	}else{
		Bucket_6[bucket_num]->recv_pong(from, remote_ID);
	}
}

void k_route_table::tick()
{
	//erase Unknown_Active endpoints that have timed out
	for(std::map<net::endpoint, k_contact>::iterator it_cur = Unknown_Active.begin();
		it_cur != Unknown_Active.end();)
	{
		if(it_cur->second.timeout()
			&& it_cur->second.timeout_count() > protocol_udp::retransmit_limit)
		{
			LOG << "timeout: " << it_cur->first.IP() << " " << it_cur->first.port();
			Unknown_Active.erase(it_cur++);
		}else{
			++it_cur;
		}
	}

	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_4[x]->tick();
		Bucket_6[x]->tick();
	}
}
