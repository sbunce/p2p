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
	connection_manager(share & Share_in);

	/*
	connect_call_back:
		Proactor does call back on this function when new connection.
	disconnect_call_back:
		Proactor does call back on this function when disconnect.
	failed_connect_call_back:
		Proactor does call back on this function when a connection attempt fails.
	*/
	void connect_call_back(network::sock & S);
	void disconnect_call_back(network::sock & S);
	void failed_connect_call_back(network::sock & S);

private:
	share & Share;

	/*
	Connection state will be stored in the connection object. Also call backs
	will be registered in connect_call_back to do call backs directly to the
	connection objects stored in this container. The mutex locks access to this
	container because it's possible there will be multiple threads doing call
	backs to connect_call_back and disconnect_call_back concurrently which both
	modify the Connection container.
	*/
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;
};
#endif
