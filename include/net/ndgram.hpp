#ifndef H_NET_NDGRAM
#define H_NET_NDGRAM

//custom
#include "protocol.hpp"
#include "socket_base.hpp"

namespace net{
class ndgram : public socket_base
{
	//max we attempt to send/recv in one go
	static const int MTU = 1536;
public:
	ndgram();                   //open but don't bind (used for send only)
	ndgram(const endpoint & E); //open and bind (used for send/recv)

	/*
	local_IP:
		Returns local port, or empty string if error.
		Postcondition: If error then close() called.
	local_port:
		Returns local port, or empty string if error.
		Postcondition: If error then close() called.
	open:
		Open for sending/receiving, endpoint is local. Example:
		Accept only from localhost. Choose random port to listen on.
			net::get_endpoint("localhost", "0", net::udp);
		Accept connections on all interfaces. Use port 1234.
			net::get_endpoint("", "1234", net::udp);
	recv:
		Receive data. Returns number of bytes received or 0 if the host
		disconnected. E is set to the endpoint of the host that sent the data. E
		may be empty if error.
	send:
		Writes bytes from buffer. Returns the number of bytes sent or 0 if the
		host disconnected. The sent bytes are erased from the buffer.
	*/
	std::string local_IP();
	std::string local_port();
	virtual void open(const endpoint & E);
	int recv(net::buffer & buf, boost::shared_ptr<endpoint> & E);
	int send(net::buffer & buf, const endpoint & E);
};
}//end of namespace net
#endif
