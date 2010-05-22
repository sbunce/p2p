#ifndef H_KAD
#define H_KAD

//custom
#include "db_all.hpp"
#include "exchange_udp.hpp"
#include "k_bucket.hpp"
#include "k_find.hpp"
#include "k_func.hpp"
#include "k_route_table.hpp"
#include "k_token.hpp"

//include
#include <atomic_int.hpp>
#include <bit_field.hpp>
#include <net/net.hpp>

//standard
#include <algorithm>

class kad
{
public:
	kad();
	~kad();

	/*
	find_node:
		Find node.
		Note: Call back may be used multiple times if more than one host has ID.
	find_file:
		Find hosts with file.
	store_file:
		Store file hash.
	*/
	void find_node(const std::string & ID,
		const boost::function<void (const net::endpoint &)> & call_back);
	void find_file(const std::string & hash,
		const boost::function<void (const net::endpoint &)> & call_back);
	void store_file(const std::string & hash);

	/* Info
	count:
		Returns number of active contacts in all k_buckets.
	download_rate:
		Returns average download rate (bytes/second).
	upload_rate:
		Returns average upload rate (bytes/second).
	*/
	unsigned count();
	unsigned download_rate();
	unsigned upload_rate();

private:
	boost::thread network_thread;
	const std::string local_ID;      //our node ID
	atomic_int<unsigned> active_cnt; //number of active contacts in k_buckets
	exchange_udp Exchange;
	k_find Find;
	k_route_table Route_Table;
	k_token Token;

	/*
	The first time we call send_ping() we want to ping all nodes we can to update
	our routing table ASAP.
	*/
	bool send_ping_called;

	/*
	Function call proxy. A thread adds a function object and network_thread makes
	the function call. This eliminates a lot of locking.
	*/
	boost::mutex relay_job_mutex;
	std::deque<boost::function<void ()> > relay_job;

	/* Relay Functions
	Called from relay_job. Refer to public function comments.
	*/
	void find_node_relay(const std::string ID,
		const boost::function<void (const net::endpoint &)> call_back);
	void find_file_relay(const std::string hash,
		const boost::function<void (const net::endpoint &)> call_back);
	void store_file_relay(const std::string hash);

	/*
	find_file_call_back_0:
		Call back to send query_file to closest nodes.
	find_file_call_back_1:
		Receives node list that was expected by find_file_call_back_0. Starts
		searches for hosts in node_list.
	network_loop:
		Loop to handle timed events and network events.
	process_relay_job:
		Called by network_thread to process relay jobs.
	route_table_call_back:
		Called when active contact added to the routing table.
	send_store_node_call_back_0:
		Call back to send store_node to closest nodes.
	send_store_node_call_back_1:
		If we don't have a store token in send_store_node_back_0 we send a ping
		and register this function as the response handler.
	store_file_call_back_0:
		Call back to send store_file to closest nodes.
	store_file_call_back_1:
		If we don't have a store token in store_file_call_back_0 we send a ping
		and register this function as the response handler.
	*/
	void find_file_call_back_0(const net::endpoint & ep,
		const std::string hash,
		const boost::function<void (const net::endpoint &)> call_back,
		boost::shared_ptr<std::set<std::string> > node_list_memoize);
	void find_file_call_back_1(const net::endpoint & from,
		const net::buffer & random, const std::string & remote_ID,
		const std::list<std::string> & nodes,
		const boost::function<void (const net::endpoint &)> call_back,
		boost::shared_ptr<std::set<std::string> > node_list_memoize);
	void network_loop();
	void process_relay_job();
	void route_table_call_back(const net::endpoint & ep, const std::string & remote_ID);
	void send_store_node_call_back_0(const net::endpoint & ep);
	void send_store_node_call_back_1(const net::endpoint & from,
		const net::buffer & random, const std::string & remote_ID);
	void store_file_call_back_0(const net::endpoint & ep, const std::string hash);
	void store_file_call_back_1(const net::endpoint & from,
		const net::buffer & random, const std::string & remote_ID,
		const std::string hash);

	/* Timed Functions
	Called by network_thread on regular time intervals.
	send_find_node:
		Send find_node messages.
	send_ping:
		Send ping messages.
	send_store_node:
		Sends store_node message.
	store_token_timeout:
		Checks timeouts on all store_tokens.
	*/
	void send_find_node();
	void send_ping();
	void send_store_node();

	/* Receive Call Back
	Functions named after the message they receive.
	*/
	void recv_find_node(const net::endpoint & from,
		const net::buffer & random, const std::string & remote_ID,
		const std::string & ID_to_find);
	void recv_host_list(const net::endpoint & from,
		const std::string & remote_ID, const std::list<net::endpoint> & hosts,
		const std::string ID_to_find);
	void recv_ping(const net::endpoint & from, const net::buffer & random,
		const std::string & remote_ID);
	void recv_pong(const net::endpoint & from, const net::buffer & random,
		const std::string & remote_ID);
	void recv_query_file(const net::endpoint & from, const net::buffer & random,
		const std::string & remote_ID, const std::string & hash);
	void recv_store_file(const net::endpoint & from, const net::buffer & random,
		const std::string & remote_ID, const std::string & hash);
	void recv_store_node(const net::endpoint & from, const net::buffer & random,
		const std::string & remote_ID);
};
#endif
