#ifndef H_NET_LISTENER
#define H_NET_LISTENER

//custom
#include "nstream.hpp"
#include "socket_base.hpp"

//include
#include <boost/shared_ptr.hpp>

namespace net{
class listener : public socket_base
{
public:
	listener();
	listener(const endpoint & ep);

	/*
	accept:
		Block until connection can be accepted. Return shared_ptr to nstream or
		empty shared_ptr if error. If the socket is non-blocking then emtpy
		shared_ptr is returned if there are no more incoming connections.
		Precondition: is_open() = true.
	open:
		Start listener listening. The endpoint has a different purpose than with
		nstream. With a nstream we are connect to a endpoint on another host.
		With a listener we are the endpoint.
		Precondition: E must be tcp endpoint.
		Example:
		Only accept connections from localhost. Choose random port to listen on.
			net::get_endpoint("localhost", "0");
		Accept connections on all interfaces. Use port 1234.
			net::get_endpoint("", "1234");
	*/
	boost::shared_ptr<nstream> accept();
	virtual void open(const endpoint & ep);
};
}//end of namespace net
#endif
