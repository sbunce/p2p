#ifndef H_NETWORK_CONNECTION_INFO
#define H_NETWORK_CONNECTION_INFO

//custom
#include "protocol.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

namespace network{
class proactor;
class connection_info : private boost::noncopyable
{
	friend class proactor;
public:
	connection_info(
		const int connection_ID_in,
		const int socket_FD_in,
		const std::string & host_in,
		const std::string & IP_in,
		const std::string & port_in,
		const protocol Protocol_in
	):
		connection_ID(connection_ID_in),
		socket_FD(socket_FD_in),
		host(host_in),
		IP(IP_in),
		port(port_in),
		Protocol(Protocol_in)
	{}

	const int connection_ID; //unique identifier for connection
	const std::string host;  //unresolved host name
	const std::string IP;    //resolved host name
	const std::string port;  //port on remote host
	const protocol Protocol; //tcp or udp

	//must be set by connect call back
	boost::function<void (connection_info &, buffer &)> recv_call_back;

	/*
	This can be set to get a call back when the send_buf size decreases.
	Note: This is optional.
	*/
	boost::function<void (connection_info &, int)> send_call_back;

private:
	//socket file descriptor only used internally to proactor
	const int socket_FD;
};
}
#endif
