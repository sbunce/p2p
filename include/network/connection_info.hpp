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
		const dir direction_in
	):
		connection_ID(connection_ID_in),
		socket_FD(socket_FD_in),
		host(host_in),
		IP(IP_in),
		port(port_in),
		direction(direction_in),
		latest_recv(0),
		latest_send(0),
		send_buf_size(0)
	{}

	const int connection_ID; //unique identifier for connection
	const std::string host;  //unresolved host name
	const std::string IP;    //remote IP
	const std::string port;  //remote port
	const dir direction;

	/*
	The recv_call_back must be set in the connect call back to recieve incoming
	data. If the recv_call_back is not set during the connect call back then
	incoming data will be discarded.
	*/
	boost::function<void (connection_info &)> recv_call_back;
	boost::function<void (connection_info &)> send_call_back;

	/*
	recv'd data will be appended to this buffer before recv_call_back called. The
	number of bytes appended are stored in latest_recv.
	*/
	buffer recv_buf;
	unsigned latest_recv;
	unsigned latest_send;

	/*
	The size of the send_buf only accessible to the proactor. This value can be
	really old if checked during the recv_call_back. This value is most up to
	date when checked during the send_call_back. If the send_call_back is set
	then a call back will happen whenever this value decreases.
	*/
	unsigned send_buf_size;

private:
	//socket file descriptor only used internally to proactor
	const int socket_FD;
};
}
#endif
