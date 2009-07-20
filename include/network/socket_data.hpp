#ifndef H_NETWORK_SOCKET_DATA
#define H_NETWORK_SOCKET_DATA

//custom
#include "buffer.hpp"
#include "wrapper.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

//standard
#include <string>

namespace network{
/*
NONE:
	Default value of Error.
MAX_CONNECTIONS:
	Reached the maximum number of possible connections.
FAILED_DNS_RESOLVE:
	DNS resolution failed.
TIMED_OUT:
	Socket disconnected due to inactivity.
*/
//windows has ERROR defined somewhere
#ifdef _WIN32
	#ifdef ERROR
		#undef ERROR
	#endif
#endif
enum ERROR {
	NONE,
	MAX_CONNECTIONS,
	FAILED_DNS_RESOLUTION,
	TIMED_OUT,
	UNKNOWN
};

/*
Direction of a connection. An incoming connection is one a remote host
established with us. An outgoing connection is one we establish with a remote
host.
*/
enum DIRECTION {
	INCOMING,
	OUTGOING
};

class reactor;
class reactor_select;

/*
This is all the state that needs to be associated with a socket.
*/
class socket_data
{
	friend class reactor;
	friend class reactor_select;
public:
	socket_data(
		const int socket_FD_in,
		const std::string & host_in,
		const std::string & IP_in,
		const std::string & port_in,
		boost::shared_ptr<wrapper::info> Info_in
			= boost::shared_ptr<wrapper::info>(new wrapper::info(NULL, NULL))
	):
		socket_FD(socket_FD_in),
		host(host_in),
		IP(IP_in),
		port(port_in),
		Info(Info_in),
		failed_connect_flag(false),
		connect_flag(false),
		recv_flag(false),
		send_flag(false),
		disconnect_flag(false),
		error(NONE),
		last_seen(std::time(NULL)),
		localhost((IP.find("127.") == 0) || (IP == "::1")),
		direction(host.empty() ? INCOMING : OUTGOING)
	{}

	const int socket_FD;       //should not be used except by reactor_*
	const std::string host;    //name we connected to (ie "google.com")
	const std::string IP;      //IP host resolved to
	const std::string port;    //if listen_port == port then connection is incoming
	const bool localhost;      //true if connection to or from localhost
	const DIRECTION direction;

	//if this is true when the call back returns the socket will be disconnected
	bool disconnect_flag;

	//send()/recv() buffers
	buffer recv_buff;
	buffer send_buff;

	/*
	If there is an error when a socket disconnects or fails to connect it will
	be stored here.
	*/
	ERROR error;

	/*
	These must be set in the connect call back or the program will be
	terminated.
	*/
	boost::function<void (socket_data &)> recv_call_back;
	boost::function<void (socket_data &)> send_call_back;

private:
	//last time seen (used for timeouts)
	std::time_t last_seen;

	/*
	failed_connect_flag:
		If true the failed connect call back needs to be done. No other call
		backs should be done after the failed connect call back.
	connect_flag:
		If false the connect call back needs to be done. After the call back is
		done this is set to true.
	recv_flag:
		If true the recv call back needs to be done.
	send_flag:
		If true the send call back needs to be done.	
	*/
	bool failed_connect_flag;
	bool connect_flag;
	bool recv_flag;
	bool send_flag;

	/*
	This is needed if the host resolves to multiple IPs. This is used to try
	the next IP in the list until connection happens or we run out of IPs. 
	However, if the first IP is connected to, no other IP's will be connected
	to.
	*/
	boost::shared_ptr<wrapper::info> Info;
};
}//end of network namespace
#endif
