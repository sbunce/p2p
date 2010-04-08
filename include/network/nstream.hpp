#ifndef H_NETWORK_NSTREAM
#define H_NETWORK_NSTREAM

//custom
#include "endpoint.hpp"
#include "system_include.hpp"

//include
#include <boost/utility.hpp>

//standard
#include <set>

namespace network{
class nstream : private boost::noncopyable
{
	//max we attempt to send/recv in one go
	static const int MTU = 8192;
public:
	nstream();
	explicit nstream(const endpoint & E);     //sync connect to endpoint
	explicit nstream(const int socket_FD_in); //create out of already connected socket
	~nstream();

	/*
	close:
		Close the socket.
	is_open:
		Returns true if connected.
		WARNING: This function returns a false positive if a socket is asynchronously
			connecting.
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
	set_non_blocking:
		Sets the socket to non-blocking.
	socket:
		Returns socket file descriptor, or -1 if disconnected.
	*/
	virtual void close();
	virtual bool is_open() const;
	virtual bool is_open_async();
	virtual std::string local_IP();
	virtual std::string local_port();
	virtual void open(const endpoint & E);
	virtual void open_async(const endpoint & E);
	virtual int recv(buffer & buf, const int max_transfer = MTU);
	virtual std::string remote_IP();
	virtual std::string remote_port();
	virtual int send(buffer & buf, int max_transfer = MTU);
	virtual void set_non_blocking();
	virtual int socket();

private:
	//-1 if not connected, or >= 0 if connected
	int socket_FD;
};
}
#endif
