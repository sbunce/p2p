#ifndef H_NET_SOCKET_BASE
#define H_NET_SOCKET_BASE

//custom
#include "buffer.hpp"
#include "endpoint.hpp"

//include
#include <boost/optional.hpp>
#include <boost/utility.hpp>

namespace net{
class socket_base : private boost::noncopyable
{
public:
	//max to attempt to send/recv in one go
	static const unsigned MTU = 1412;

	socket_base();
	virtual ~socket_base();

	/*
	close:
		Close the socket.
	is_open:
		Returns true if socket is open.
	local_ep:
		Returns local endpoint or nothing if error.
		Postcondition: If error then socket closed.
	open:
		Different meaning depending on derived class.
	remote_ep:
		Returns remote endpoint or nothing if error.
		Postcondition: If error then socket closed.
	set_non_block:
		Set the socket to non blocking, or set the socket to blocking. Returns
		false if failed.
	socket:
		Returns socket file descriptor.
	*/
	virtual void close();
	virtual bool is_open() const;
	virtual boost::optional<endpoint> local_ep();
	virtual void open(const net::endpoint & E) = 0;
	virtual boost::optional<endpoint> remote_ep();
	virtual bool set_non_blocking(const bool val);
	virtual int socket();

protected:
	//-1 if not connected, or >= 0 if connected
	int socket_FD;
};
}//end namespace net
#endif
