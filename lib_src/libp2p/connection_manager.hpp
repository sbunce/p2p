#ifndef H_CONNECTION_MANAGER
#define H_CONNECTION_MANAGER

//custom
#include "connection.hpp"
#include "kad.hpp"

//include
#include <atomic_int.hpp>
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
	add:
		Find hosts which have the file and connect.
	remove:
		Remove incoming/outgoing slots with the specified hash.
	store:
		Store file hash on the DHT.
	*/
	void add(const std::string & hash);
	void remove(const std::string & hash);
	void store_file(const std::string & hash);

	/* Info
	connections:
		Returns number of established connections.
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
	unsigned connections();
	unsigned DHT_count();
	unsigned TCP_download_rate();
	unsigned TCP_upload_rate();
	unsigned UDP_download_rate();
	unsigned UDP_upload_rate();

	//set options
	void set_max_download_rate(const unsigned rate);
	void set_max_connections(const unsigned incoming_limit, const unsigned outgoing_limit);
	void set_max_upload_rate(const unsigned rate);

	/*
	start:
		Start networking.
	*/
	void start();

private:
	net::proactor Proactor;
	thread_pool Thread_Pool;
	kad DHT;
	atomic_int<unsigned> _connections;

	class address
	{
	public:
		address(const std::string & IP_in, const std::string & port_in);
		address(const address & A);
		std::string IP;
		std::string port;
		bool operator < (const address & rval) const;
	};

	class connection_element
	{
	public:
		connection_element(
			const address & Address_in,
			const boost::shared_ptr<connection> & Connection_in
		);
		connection_element(const connection_element & CE);
		address Address;
		boost::shared_ptr<connection> Connection;
	};

	/*
	Connect_mutex:
		Locks access to all in this section.
	Connection:
		The connection_ID associated with a connection (the state each connection
		needs). The Connection_mutex locks all access to the Connection container.
	Connecting:
		Hosts we're connecting to.
	Connected:
		Only hosts that are connected. Address associated with connection_ID.
	*/
	boost::mutex Connect_mutex;
	std::map<int, connection_element> Connection;
	std::set<address> Connecting;
	std::set<address> Connected;


	//std::map<std::pair<std::string, std::string>, std::string>

	//memoize ticks we've scheduled so we don't do redundant ticks
	boost::mutex tick_memoize_mutex;
	std::set<int> tick_memoize;

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
	add_call_back:
		Call back used by add() function to connect to hosts.
	remove_priv:
		Removes downloading file. Scheduled by remove().
	tick:
		Do tick() of connection if it exists.
	trigger_tick:
		Schedule a job with the thread pool to tick a connection.
	*/
	void add_call_back(const net::endpoint & ep);
	void remove_priv(const std::string hash);
	void tick(const int connection_ID);
	void trigger_tick(const int connection_ID);
};
#endif
