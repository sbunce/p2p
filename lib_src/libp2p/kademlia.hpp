#ifndef H_KADEMLIA
#define H_KADEMLIA

//custom
#include "bucket.hpp"
#include "db_all.hpp"
#include "exchange_udp.hpp"
#include "kad_func.hpp"
#include "route_table.hpp"

//include
#include <bit_field.hpp>
#include <network/network.hpp>

class kademlia
{
public:
	kademlia();
	~kademlia();

	/*

	*/
	//void find_node(const std::string & ID,
		//const boost::function<void (const network::endpoint)> & call_back);

private:
	boost::thread network_thread;
	exchange_udp Exchange;
	const std::string local_ID;
	route_table Route_Table;

	/*
	Function call proxy. A thread adds a function object and network_thread makes
	the function call. This eliminates a lot of locking.
	*/
	boost::mutex relay_job_mutex;
	std::deque<boost::function<void ()> > relay_job;

	/*
	network_loop:
		Loop to handle timed events and network events
	process_relay_job:
		Called by network_thread to process relay jobs.
	send_ping:
		Pings hosts which are about to timeout.
	*/
	void network_loop();
	void process_relay_job();
	void send_ping();

	/* Receive Functions
	Called when message received. Function named after message type it handles.
	Only unusual functions are documented.
	recv_ping:
		Received ping.
	recv_pong:
		Received pong that's result of pinging contact in k_bucket.
	recv_pong_bucket_reserve:
		Received pong that's result of pinging contact in bucket_reserve.
	recv_pong_unknown_reserve:
		Received pong that's result of pinging contact in unknown_reserve.
	*/
	void recv_find_node(const network::endpoint & endpoint,
		const network::buffer & random, const std::string & remote_ID,
		const std::string & ID_to_find);
	void recv_ping(const network::endpoint & endpoint, const network::buffer & random,
		const std::string & remote_ID);
	void recv_pong(const network::endpoint & endpoint, const std::string & remote_ID);
	void recv_pong_bucket_reserve(const network::endpoint & endpoint, const std::string & remote_ID,
		const unsigned expected_bucket_num);
};
#endif
