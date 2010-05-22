#ifndef H_CONNECTION_MANAGER
#define H_CONNECTION_MANAGER

//custom
#include "connection.hpp"
#include "kad.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <net/net.hpp>
#include <thread_pool.hpp>

//standard
#include <map>

class connection_manager : private boost::noncopyable
{
public:
	connection_manager();
	~connection_manager();

	/*
	remove:
		Remove incoming/outgoing slots with the specified hash.
	start:
		Start networking.
	*/
	void remove(const std::string & hash);
	void start();

	/* Info
	DHT_count:
		Returns number of contacts in DHT routing table.
	TCP_download_rate:
		Returns current average TCP download rate (B/s).
	TCP_upload_rate:
		Returns current average TCP upload rate (B/s).
	UDP_download_rate:
		Returns current average UDP download rate (B/s).
	UDP_upload_rate:
		Returns current average UDP upload rate (B/s).
	*/
	unsigned DHT_count();
	unsigned TCP_download_rate();
	unsigned TCP_upload_rate();
	unsigned UDP_download_rate();
	unsigned UDP_upload_rate();

	//set options
	void set_max_download_rate(const unsigned rate);
	void set_max_connections(const unsigned incoming_limit, const unsigned outgoing_limit);
	void set_max_upload_rate(const unsigned rate);

private:
	net::proactor Proactor;
	thread_pool Thread_Pool;
	kad DHT;

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

	/* Proactor Call Backs
	connect_call_back:
		Proactor does call back when there is a new connection.
	disconnect_call_back:
		Proactor does call back when new connection failed, or when existing
		connection closed.
	*/
	void connect_call_back(net::proactor::connection_info & CI);
	void disconnect_call_back(net::proactor::connection_info & CI);

	/*
	remove_priv:
		Removes downloading file. Scheduled by remove().
	tick:
		Do tick() of connection if it exists.
	trigger_tick:
		Schedule a job with the thread pool to tick a connection.

	*/
	void remove_priv(const std::string hash);
	void tick(const int connection_ID);
	void trigger_tick(const int connection_ID);
};
#endif
