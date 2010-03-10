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
	static const unsigned bucket_count = SHA1::bin_size * 8;
	static const unsigned max_bucket_size = 20;

	kademlia();
	~kademlia();

private:
	boost::thread main_loop_thread;
	exchange_udp Exchange;
	const std::string local_ID;

	//the k-buckets, the kademlia routing table
	//k_bucket Bucket[bucket_count];

	/*
	calc_bucket:
		Returns bucket number that ID belongs in.
	network_loop:
		Loop to handle timed events and network recv.
	process_bucket_cache:
		Called once per second to add nodes to k-buckets.
	*/
	unsigned calc_bucket(const std::string & remote_ID);
	void main_loop();
	void process_bucket_cache();

	/* Receive Functions
	Called when message received. Function named after message type it handles.
	*/
	void recv_ping(const network::buffer & random, const std::string & remote_ID);
	void recv_pong(const std::string & remote_ID);
};
#endif
