#ifndef H_NET_NSTREAM
#define H_NET_NSTREAM

//custom
#include "socket_base.hpp"

namespace net{
class nstream : public socket_base
{
public:
	nstream();
	explicit nstream(const endpoint & ep);     //sync connect to endpoint
	explicit nstream(const int socket_FD_in); //create out of already connected socket

	/*
	is_open_async:
		After open_async we must wait for the socket to become writeable. When it
		is we can call this to see if the connection succeeded.
		WARNING: If this is called before the socket becomes writeable we can have
			a false negative.
	local_ep:
		Returns local endpoint. Returns nothing if not connected, or if ep could
		not be determined.
		Note: endpoints returned for current or latest connection.
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
	remote_ep:
		Returns remote endpoint, or nothing if couldn't be determined.
	send:
		Write bytes from buffer. Returns the number of bytes sent, 0 if the
		host disconnected, or -1 on error. The sent bytes are erased from the
		buffer.
	*/
	bool is_open_async();
	boost::optional<endpoint> local_ep();
	virtual void open(const endpoint & ep);
	void open_async(const endpoint & ep);
	int recv(buffer & buf, const int max_transfer = MTU);
	boost::optional<endpoint> remote_ep();
	int send(buffer & buf, int max_transfer = MTU);

private:
	boost::optional<endpoint> _local_ep;
	boost::optional<endpoint> _remote_ep;

	/*
	local_IP:
		Returns local IP or empty string if error.
		Postcondition: If error connection is closed.
	local_port:
		Returns local port or empty string if error.
		Postcondition: If error connection is closed.
	remote_IP:
		Returns remote IP or empty string if error.
		Postcondition: If error connection is closed.
	remote_port:
		Returns port or empty string if error.
		Postcondition: If error connection is closed.
	set_local_ep:
		Called when socket connects to set _local_ep;
	set_remote_ep:
		We only call this in the ctor that takes an already connected socket. We
		don't need to call this on an endpoint we have open()'d because we already
		know the endpoint.
	*/
	std::string local_IP();
	std::string local_port();
	std::string remote_IP();
	std::string remote_port();
	void set_local_ep();
	void set_remote_ep();
};
}//end of namespace net
#endif
