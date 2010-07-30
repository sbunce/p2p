#ifndef H_K_ROUTE_TABLE
#define H_K_ROUTE_TABLE

//custom
#include "k_bucket.hpp"
#include "k_contact.hpp"
#include "db_all.hpp"

//include
#include <atomic_int.hpp>

//standard
#include <algorithm>

class k_route_table
{
public:
	/*
	active_cnt:
		Count of how many active contacts exist in k_buckets.
	route_table_call_back:
		Called back whenever a active node added.
	*/
	k_route_table(
		atomic_int<unsigned> & active_cnt,
		const boost::function<void (const net::endpoint &, const std::string &)> & route_table_call_back
	);

	/*
	add_reserve:
		Add endpoint to reserve. Nodes that go in to a k_bucket start in reserve.
	find_node:
		Returns endpoints closest to ID_to_find. The returned list is suitable to
		pass to the message_udp::send::host_list ctor.
	find_node_local:
		Like the above function but used for local find_node search. This function
		returns distance in addition to endpoint.
	ping:
		Returns endpoint to be pinged.
	recv_pong:
		Call back used when pinging endpoint in Unknown. This call back determines
		what bucket the endpoint belongs in.
	*/
	void add_reserve(const net::endpoint & ep, const std::string & remote_ID = "");
	std::list<net::endpoint> find_node(const net::endpoint & from,
		const std::string & ID_to_find);
	std::multimap<mpa::mpint, net::endpoint> find_node_local(const std::string & ID_to_find);
	boost::optional<net::endpoint> ping();
	void recv_pong(const net::endpoint & from, const std::string & remote_ID);

	/* Timed Functions
	tick:
		Called once per second to process timeouts.
	*/
	void tick();

private:
	const std::string local_ID;

	//k_buckets for IPv4 and IPv6
	boost::scoped_ptr<k_bucket> Bucket_4[protocol_udp::bucket_count];
	boost::scoped_ptr<k_bucket> Bucket_6[protocol_udp::bucket_count];

	/*
	Unknown_Active:
		When unknown node is pinged it is placed here, along with the time at
		which it was pinged.
	Unknown_Reserve:
		When we don't know the bucket the endpoint belongs in it's placed here.
	*/
	std::map<net::endpoint, k_contact> Unknown_Active;
	std::set<net::endpoint> Unknown_Reserve;
};
#endif
