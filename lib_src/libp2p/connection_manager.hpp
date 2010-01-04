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
private:
	/*
	The connection_ID associated with a connection (the state each connection
	needs). The Connection_mutex locks all access to the Connection container.
	*/
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;

public:
	connection_manager();

	/*
	This needs to be constructed last because it will do call backs which modify
	Connection map.
	*/
	network::proactor Proactor;

	/* Fixed call backs registered with proactor ctor.
	connect_call_back:
		Proactor does call back when there is a new connection.
	disconnect_call_back:
		Proactor does call back when new connection failed, or when existing
		connection closed.
	*/
	void connect_call_back(network::connection_info & CI);
	void disconnect_call_back(network::connection_info & CI);
};
#endif
