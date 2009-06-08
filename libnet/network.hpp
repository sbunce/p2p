#ifndef H_NETWORK
#define H_NETWORK

/*
Select which reactor to use on each OS. These must correspond with the
instantiated reactor in network::io_service private: section.
*/
#ifdef LINUX2
	#include "reactor_select.hpp"
#elif WIN32
	#include "reactor_select.hpp"
#endif

namespace network{
typedef socket_data::socket_data_visible socket;
class io_service
{
public:
	/* ctor parameters
	boost::bind is the nicest way to do a callback to the member function of a
	specific object. ex:
		boost::bind(&http::connect_call_back, &HTTP, _1, _2, _3)

	connect_call_back:
		Called when server connected to. The direction indicates whether we
		established connection (direction OUTGOING), or if someone established a
		connection to us (direction INCOMING). The send_buff is to this function
		in case something needs to be sent after connect.
	recv_call_back:
		Called after data received (received data in recv_buff). A return value of 
		true will keep the server connected, but false will cause it to be
		disconnected.
	send_call_back:
		Called after data sent. The send_buff will contain data that hasn't yet
		been sent. A return value of true will keep the server connected, but
		false will cause it to be disconnected.
	disconnect_call-back:
		Called when a connected socket disconnects.
	connect_time_out:
		Seconds before connection attempt times out.
	socket_time_out:
		Seconds before socket with no I/O times out.
	listen_port:
		Port to listen on for incoming connections. The default (if no parameter
		specified) is to not listen for incoming connections.
	*/
	io_service(
		boost::function<void (socket_data::socket_data_visible &)> failed_connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible &)> connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible &)> disconnect_call_back_in
	):
		Reactor(
			failed_connect_call_back_in,
			connect_call_back_in,
			disconnect_call_back_in
		)
	{}

	//asynchronously connect to host
	void connect(const std::string & host, const std::string & port)
	{
		Reactor.connect(host, port);
	}

private:
	//select which reactor to use on each OS
	#ifdef LINUX2
	reactor_select Reactor;
	#elif WIN32
	reactor_select Reactor;
	#endif
};
}//end network namespace
#endif
