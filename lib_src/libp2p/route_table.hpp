#ifndef H_ROUTE_TABLE
#define H_ROUTE_TABLE

//custom
#include "bucket.hpp"
#include "db_all.hpp"

class route_table
{
public:
	route_table();

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
	void add_reserve(const std::string & remote_ID, const network::endpoint & endpoint);
	std::list<std::pair<network::endpoint, unsigned char> > find_node(
		const std::string & remote_ID, const std::string & ID_to_find);
	boost::optional<network::endpoint> ping();
	void pong(const std::string & remote_ID, const network::endpoint & endpoint);

private:
	const std::string local_ID;

	bucket Bucket_4[protocol_udp::bucket_count];
	bucket Bucket_6[protocol_udp::bucket_count];
};
#endif
