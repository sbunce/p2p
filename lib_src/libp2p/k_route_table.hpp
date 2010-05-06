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
		See bucket::add_reserve for documentation.
	find_node:
		Returns nodes closest to ID_to_find. The returned list is suitable to pass
		to the message_udp::send::host_list ctor.
	ping:
		Returns endpoint to ping.
	pong:
		See bucket::pong for documentation.
	*/
	void add_reserve(const std::string & remote_ID, const net::endpoint & endpoint);
	std::list<std::pair<net::endpoint, unsigned char> > find_node(
		const std::string & remote_ID, const std::string & ID_to_find);
	boost::optional<net::endpoint> ping();
	void pong(const std::string & remote_ID, const net::endpoint & endpoint);

private:
	const std::string local_ID;

	k_bucket Bucket_4[protocol_udp::bucket_count];
	k_bucket Bucket_6[protocol_udp::bucket_count];
};
#endif
