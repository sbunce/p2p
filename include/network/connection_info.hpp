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
class connection_info : private boost::noncopyable
{
public:
	connection_info(
		const int socket_FD_in,
		const std::string & host_in,
		const std::string & IP_in,
		const std::string & port_in,
		const protocol Protocol_in
	):
		socket_FD(socket_FD_in),
		host(host_in),
		IP(IP_in),
		port(port_in),
		Protocol(Protocol_in)
	{}

//DEBUG, needs to be replaced with connection_ID so call back dispatcher memoize
//will work properly
//Also, create connect_call_back function pointer container. So clients that
//connect first get called back first.
	const int socket_FD;
	const std::string host;
	const std::string IP;
	const std::string port;
	const protocol Protocol;

	//must be set by connect call back
	boost::function<void (connection_info &, buffer &)> recv_call_back;

	/*
	This can be set to get a call back when the send_buf size decreases.
	Note: This is optional.
	*/
	boost::function<void (connection_info &, int)> send_call_back;
};
}
#endif
