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
	listener(const endpoint & ep); //open listener on local endpoint

	/*
	accept:
		Blocks until connection can be accepted. Returns shared_ptr to nstream or
		empty shared_ptr if error. If the socket is non-blocking then an emtpy
		shared_ptr is returned if there are no more incoming connections.
		Precondition: is_open() = true.
	open:
		Start listener listening. The endpoint has a different purpose than with
		nstream. With a nstream we are connect to a endpoint on another host.
		With a listener we are the endpoint.
		Precondition: E must be tcp endpoint.
		Example:
		Only accept connections from localhost. Choose random port to listen on.
			net::get_endpoint("localhost", "0", net::tcp);
		Accept connections on all interfaces. Use port 1234.
			net::get_endpoint("", "1234", net::tcp);
	local_ep:
		Returns endpoint listening on, or endpoint we attempted to listen on.
	*/
	boost::shared_ptr<nstream> accept();
	virtual void open(const endpoint & ep);
	boost::optional<endpoint> local_ep();

private:
	boost::optional<endpoint> _local_ep;

	/*
	set_local_ep:
		Set _local_ep to endpoint we're listening on, or don't set if if listen
		failed.
	local_IP:
		Return local IP listening on or empty string if not listening.
	local_port:
		Return local port listening on or emtpy string if not listening.
	*/

	void set_local_ep();
	std::string local_IP();
	std::string local_port();
};
}//end of namespace net
#endif
