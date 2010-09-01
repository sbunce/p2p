#ifndef H_NET_NSTREAM
#define H_NET_NSTREAM

//custom
#include "socket_base.hpp"

namespace net{
class nstream : public socket_base
{
public:
	nstream();
	explicit nstream(const endpoint & ep);    //sync connect to endpoint
	explicit nstream(const int socket_FD_in); //create out of already connected socket

	/*
	is_open_async:
		After open_async we must wait for the socket to become writeable. When it
		is we can call this to see if the connection succeeded.
		WARNING: If this is called before the socket becomes writeable there can
			be a false positive.
	open:
		Opens connection. Blocks until connection established.
		Postcondition: local_ep() will return endpoint if no error.
	open_async:
		Start async connection. The socket must be monitored by select. When it
		becomes writeable is_open_async() should be called to determine if the
		socket connected, or failed to connect.
		Postcondition: local_ep() will return endpoint if no error.
	recv:
		Read bytes in to buffer. Returns the number of bytes read, 0 if the host
		disconnected, or -1 on error.
	send:
		Write bytes from buffer. Returns the number of bytes sent, 0 if the
		host disconnected, or -1 on error. The sent bytes are erased from the
		buffer.
	*/
	bool is_open_async();
	virtual void open(const endpoint & ep);
	void open_async(const endpoint & ep);
	int recv(buffer & buf, const int max_transfer = MTU);
	int send(buffer & buf, int max_transfer = MTU);
};
}//end of namespace net
#endif
