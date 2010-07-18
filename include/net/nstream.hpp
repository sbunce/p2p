#ifndef H_NET_NSTREAM
#define H_NET_NSTREAM

//custom
#include "socket_base.hpp"

namespace net{
class nstream : public socket_base
{
	//max we attempt to send/recv in one go
	static const int MTU = 8192;
public:
	nstream();
	explicit nstream(const endpoint & E);     //sync connect to endpoint
	explicit nstream(const int socket_FD_in); //create out of already connected socket

	/*
	is_open_async:
		After open_async we must wait for the socket to become writeable. When it
		is we can call this to see if the connection succeeded.
		WARNING: If this is called before the socket becomes writeable we can have
			a false negative.
	local_IP:
		Returns local port, or empty string if error.
		Postcondition: If error closes socket.
	local_port:
		Returns local port, or empty string if error.
		Postcondition: If error closes socket.
	open:
		Opens connection. Blocks until connection established.
	open_async:
		Start async connection. The socket must be monitored by select. When it
		becomes writeable is_open_async() should be called to determine if the
		socket connected, or failed to connect.
	recv:
		Read bytes in to buffer. Returns the number of bytes read, 0 if the host
		disconnected, or -1 if error.
	remote_IP:
		Returns local port, or empty string if error.
		Postcondition: If error closes socket.
	remote_port:
		Returns local port, or empty string if error.
		Postcondition: If error closes socket.
	send:
		Write bytes from buffer. Returns the number of bytes sent, or 0 if the
		host disconnected. The sent bytes are erased from the buffer.
	*/
	bool is_open_async();
	std::string local_IP();
	std::string local_port();
	virtual void open(const endpoint & E);
	void open_async(const endpoint & E);
	int recv(buffer & buf, const int max_transfer = MTU);
	std::string remote_IP();
	std::string remote_port();
	int send(buffer & buf, int max_transfer = MTU);
};
}//end of namespace net
#endif
