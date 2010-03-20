#ifndef H_KADEMLIA
#define H_KADEMLIA

//custom
#include "database.hpp"
#include "exchange_udp.hpp"
#include "k_bucket.hpp"

//include
#include <bit_field.hpp>
#include <network/network.hpp>

class kademlia
{
public:
	kademlia();
	~kademlia();

private:
	boost::thread main_loop_thread;
	exchange_udp Exchange;
	const std::string local_ID;

	//the k-buckets, the kademlia routing table
	k_bucket Bucket[protocol_udp::bucket_count];

	//nodes that don't fit in k_bucket
	std::set<network::endpoint> Known_Reserve[protocol_udp::bucket_count];

	//
	bit_field Known_Reserve_Ping;

	//nodes we don't know the ID of
	std::set<network::endpoint> Unknown_Reserve;

	/*
	calc_bucket:
		Returns bucket number that ID belongs in.
	do_pings:
		Pings hosts which are about to timeout.
	endpoint_exists_in_k_bucket:
		Returns true if the endpoint exists in any k_bucket.
	endpoint_exists_in_reserve:
		Returns true if endpoint exists in Known_Reserve or Unknown_Reserve.
	main_loop:
		Loop to handle timed events and network recv.
	*/
	unsigned calc_bucket(const std::string & remote_ID);
	void do_pings();
	bool endpoint_exists_in_k_bucket(const network::endpoint & endpoint);
	bool endpoint_exists_in_reserve(const network::endpoint & endpoint);
	void main_loop();

	/* Receive Functions
	Called when message received. Function named after message type it handles.
	*/
	void recv_ping(const network::endpoint & endpoint, const network::buffer & random);
	void recv_pong(const network::endpoint & endpoint, const std::string & remote_ID);

	/* Timeout Functions
	Called when a request times out.
	*/
	void timeout_ping(const network::endpoint endpoint);
};
#endif
