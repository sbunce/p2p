#ifndef H_K_ROUTE_TABLE
#define H_K_ROUTE_TABLE

//custom
#include "k_bucket.hpp"
#include "db_all.hpp"

class k_route_table
{
public:
	k_route_table();

	/*
	add_reserve:


	check_timeouts:
		Check timeouts on all k_buckets.
	find_node:
		Returns nodes closest to ID_to_find. The returned list is suitable to pass
		to the message_udp::send::host_list ctor.
	find_node_local:
		When we start searching for a node ID we start searching locally.
	ping:

	recv_pong:
		Call back used when pinging endpoint in Unknown. This call back determines
		what bucket the endpoint belongs in.
	*/
	void add_reserve(const net::endpoint & endpoint, const std::string & remote_ID = "");
	std::list<net::endpoint> find_node(const std::string & remote_ID,
		const std::string & ID_to_find);
	std::map<mpa::mpint, net::endpoint> find_node_local(const std::string & local_ID,
		const std::string & ID_to_find);
	boost::optional<net::endpoint> ping();
	void recv_pong(const net::endpoint & from, const std::string & remote_ID);

private:
	const std::string local_ID;

	//k_buckets for IPv4 and IPv6
	k_bucket Bucket_4[protocol_udp::bucket_count];
	k_bucket Bucket_6[protocol_udp::bucket_count];

	//we don't know what bucket these hosts belong to
	std::list<net::endpoint> Unknown;
};
#endif
