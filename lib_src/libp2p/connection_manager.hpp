#ifndef H_CONNECTION_MANAGER
#define H_CONNECTION_MANAGER

//custom
#include "connection.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>
#include <thread_pool.hpp>

//standard
#include <map>

class connection_manager : private boost::noncopyable
{
public:
	connection_manager();
	~connection_manager();

	network::proactor Proactor;

	/*
	connect_call_back:
		Proactor does call back when there is a new connection.
	disconnect_call_back:
		Proactor does call back when new connection failed, or when existing
		connection closed.
	remove:
		Remove incoming/outgoing slots with the specified hash.
	*/
	void connect_call_back(network::connection_info & CI);
	void disconnect_call_back(network::connection_info & CI);
	void remove(const std::string hash);

private:
	thread_pool Thread_Pool;

	/*
	The connection_ID associated with a connection (the state each connection
	needs). The Connection_mutex locks all access to the Connection container.
	*/
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;

	/*
	We use the filter set to eliminate unnecessary tick()s from being scheduled
	with the thread pool
	*/
	boost::mutex filter_mutex;
	std::set<int> filter;

	/*
	do_remove:
		Removes downloading file. Scheduled by remove().
	do_tick:
		Do tick() of connection if it exists.
	trigger_tick:
		Schedule a job with the thread pool to tick a connection.
	*/
	void do_remove(const std::string hash);
	void do_tick(const int connection_ID);
	void trigger_tick(const int connection_ID);
};
#endif
