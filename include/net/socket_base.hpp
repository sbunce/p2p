#ifndef H_NET_SOCKET_BASE
#define H_NET_SOCKET_BASE

//custom
#include "buffer.hpp"
#include "endpoint.hpp"

//include
#include <boost/utility.hpp>

namespace net{
class socket_base : private boost::noncopyable
{
public:
	socket_base();
	virtual ~socket_base();

	/*
	close:
		Close the socket.
	is_open:
		Returns true if socket is open.
	open:
		Different meaning depending on derived class.
	set_non_block:
		Set the socket to non blocking, or set the socket to blocking. Returns
		false if failed.
	*/
	virtual void close();
	virtual bool is_open() const;
	virtual void open(const net::endpoint & E) = 0;
	virtual bool set_non_blocking(const bool val);
	virtual int socket();

protected:
	//-1 if not connected, or >= 0 if connected
	int socket_FD;
};
}//end namespace net
#endif
