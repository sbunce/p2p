#ifndef H_KADEMLIA
#define H_KADEMLIA

//custom
#include "db_all.hpp"
#include "exchange_udp.hpp"
#include "k_bucket.hpp"
#include "k_func.hpp"

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

	//nodes that don't fit in a k_bucket
	std::set<network::endpoint> Bucket_Overflow[protocol_udp::bucket_count];
	unsigned Bucket_Overflow_Ping[protocol_udp::bucket_count]; //pings sent

	/*
	do_pings:
		Pings hosts which are about to timeout.
	main_loop:
		Loop to handle timed events and network recv.
	*/
	void do_pings();
	void main_loop();

	/* Receive Functions
	Called when message received. Function named after message type it handles.
	Only unusual functions are documented.
	recv_ping:
		Received ping.
	recv_pong:
		Received pong that's result of pinging contact in k_bucket.
	recv_pong_bucket_overflow:
		Received pong that's result of pinging contact in bucket_overflow.
	recv_pong_unknown_reserve:
		Received pong that's result of pinging contact in unknown_reserve.
	*/
	void recv_find_node(const network::endpoint & endpoint,
		const network::buffer & random, const std::string & remote_ID,
		const std::string & ID_to_find);
	void recv_ping(const network::endpoint & endpoint, const network::buffer & random,
		const std::string & remote_ID);
	void recv_pong(const network::endpoint & endpoint, const std::string & remote_ID);
	void recv_pong_bucket_overflow(const network::endpoint & endpoint, const std::string & remote_ID,
		const unsigned expected_bucket_num);

	/* Timeout Functions
	Called when a request times out.
	*/
	void timeout_ping_bucket(const network::endpoint endpoint, const unsigned bucket_num);
	void timeout_ping_bucket_overflow(const network::endpoint endpoint, const unsigned bucket_num);
};
#endif
