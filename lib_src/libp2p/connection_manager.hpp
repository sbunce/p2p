#ifndef H_CONNECTION_MANAGER
#define H_CONNECTION_MANAGER

//custom
#include "connection.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>

//standard
#include <map>

class connection_manager : private boost::noncopyable
{
public:
	connection_manager(network::proactor & Proactor_in);

	/* Fixed call backs registered with proactor ctor.
	connect_call_back:
		Proactor does call back when there is a new connection.
	disconnect_call_back:
		Proactor does call back when new connection failed, or when existing
		connection closed.
	*/
	void connect_call_back(network::connection_info & CI);
	void disconnect_call_back(network::connection_info & CI);

private:
	network::proactor & Proactor;

	//thread that resides in connect_loop()
	boost::thread connect_loop_thread;

	/*
	The connection_ID associated with a connection (the state each connection
	needs). The Connection_mutex locks all access to the Connection container.
	*/
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;
};
#endif
