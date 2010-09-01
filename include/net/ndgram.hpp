#ifndef H_NET_NDGRAM
#define H_NET_NDGRAM

//custom
#include "socket_base.hpp"

namespace net{
class ndgram : public socket_base
{
public:
	ndgram();                   //don't bind to local port (used for send only)
	ndgram(const endpoint & E); //open and bind to local port (used for send/recv)

	/*
	open:
		Open for sending/receiving, endpoint is local.
		Examples:
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
	virtual void open(const endpoint & E);
	int recv(net::buffer & buf, boost::optional<endpoint> & E);
	int send(net::buffer & buf, const endpoint & E);
};
}//end namespace net
#endif
