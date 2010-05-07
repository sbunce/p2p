#ifndef H_KAD
#define H_KAD

//custom
#include "db_all.hpp"
#include "exchange_udp.hpp"
#include "k_bucket.hpp"
#include "k_find.hpp"
#include "k_func.hpp"
#include "k_route_table.hpp"

//include
#include <bit_field.hpp>
#include <net/net.hpp>

class kad
{
public:
	kad();
	~kad();

	/*
	find_node:
		Find a node ID. The call back is used when the node ID is found.
	*/
	void find_node(const std::string & ID_to_find,
		const boost::function<void (const net::endpoint)> & call_back);

private:
	boost::thread network_thread;
	exchange_udp Exchange;
	const std::string local_ID;
	k_route_table Route_Table;

	/*
	Function call proxy. A thread adds a function object and network_thread makes
	the function call. This eliminates a lot of locking.
	*/
	boost::mutex relay_job_mutex;
	std::deque<boost::function<void ()> > relay_job;

	/*
	find_node_relay:
		Called as relay job. Starts the find process for a node ID.
	network_loop:
		Loop to handle timed events and network events
	process_relay_job:
		Called by network_thread to process relay jobs.
	send_ping:
		Pings hosts which are about to timeout.
	*/
	void find_node_relay(const std::string ID_to_find,
		const boost::function<void (const net::endpoint)> call_back);
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
	void recv_find_node(const net::endpoint & endpoint,
		const net::buffer & random, const std::string & remote_ID,
		const std::string & ID_to_find);
	void recv_ping(const net::endpoint & endpoint, const net::buffer & random,
		const std::string & remote_ID);
	void recv_pong(const net::endpoint & endpoint, const std::string & remote_ID);
	void recv_pong_bucket_reserve(const net::endpoint & endpoint, const std::string & remote_ID,
		const unsigned expected_bucket_num);
};
#endif
