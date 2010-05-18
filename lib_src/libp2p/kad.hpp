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
#include <atomic_int.hpp>
#include <bit_field.hpp>
#include <net/net.hpp>

class kad
{
public:
	kad();
	~kad();

	/*
	contacts:
		Returns number of active contacts in all k_buckets.
	find_node:
		Find a node ID. The call back is used when the node ID is found.
		Note: The found endpoint may not be right. The call back may be called
			multiple times for all hosts that claim to have the specified ID. When
			sure the correct endpoint has been found find_node_cancel() should be
			called.
	find_node_cancel:
		Once sure that the node has been found this function should be called to
		stop further find_node requests.
	find_set:
		Find set of nodes which are closest to ID. The call back is used when the
		set is found.
	*/
	unsigned count();
	void find_node(const std::string & ID_to_find,
		const boost::function<void (const net::endpoint &)> & call_back);
	void find_node_cancel(const std::string & ID);

private:
	boost::thread network_thread;
	const std::string local_ID;      //our node ID
	atomic_int<unsigned> active_cnt; //number of active contacts in k_buckets
	exchange_udp Exchange;
	k_route_table Route_Table;
	k_find Find;

	class store_token
	{
	public:
		enum direction_t{
			incoming,
			outgoing
		};
		explicit store_token(
			const net::buffer & random_in,
			const direction_t direction
		);
		store_token(const store_token & ST);
		//random bytes sent or received in pong message
		const net::buffer random;
		//returns true if store_token timed out
		bool timeout();
	private:
		std::time_t time;  //time when store_token instantiated
		unsigned _timeout; //timeout (seconds)
	};

	/*
	incoming_store_token:
		Stores tokens we have received from remote hosts.
	outgoing_store_token:
		Stores tokens we have sent to remote hosts.
	*/
	std::multimap<net::endpoint, store_token> incoming_store_token;
	std::multimap<net::endpoint, store_token> outgoing_store_token;

	/*
	Function call proxy. A thread adds a function object and network_thread makes
	the function call. This eliminates a lot of locking.
	*/
	boost::mutex relay_job_mutex;
	std::deque<boost::function<void ()> > relay_job;

	/* Relay Functions
	Called from relay_job. Refer to public functions to see what these do.
	*/
	void find_node_relay(const std::string ID_to_find,
		const boost::function<void (const net::endpoint &)> call_back);
	void find_node_cancel_relay(const std::string ID);

	/*
	network_loop:
		Loop to handle timed events and network events.
	process_relay_job:
		Called by network_thread to process relay jobs.
	route_table_call_back:
		Called when active contact added to the routing table.
	*/
	void network_loop();
	void process_relay_job();
	void route_table_call_back(const net::endpoint & ep, const std::string & remote_ID);

	/* Timed Functions
	send_find_node:
		Send find_node messages.
	send_ping:
		Send ping messages.
	send_store_node:
		Sends store_node message.
	send_store_node_call_back (1 parameter):
		Call back to send store_node to closest nodes.
	send_store_node_call_back (3 parameters):
		If we don't have a store token in send_store_node_call_back (1 parameter)
		we send a ping and register send_store_node_call_back (3 parameters) as
		the response handler. This function sends the store after the pong is
		received.
	store_token_timeout:
		Checks timeouts on all store_tokens.
	*/
	void send_find_node();
	void send_ping();
	void send_store_node();
	void send_store_node_call_back(const std::list<net::endpoint> & ep_list);
	void send_store_node_call_back(const net::endpoint & from,
		const net::buffer & random, const std::string & remote_ID);
	void store_token_timeout();

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
	void recv_store_node(const net::endpoint & from, const net::buffer & random,
		const std::string & remote_ID);
};
#endif
