#ifndef H_NETWORK_LISTENER
#define H_NETWORK_LISTENER

//custom
#include "nstream.hpp"

//include
#include <boost/shared_ptr.hpp>

namespace network{
class listener : private boost::noncopyable
{
public:

	listener();
	listener(const endpoint & E); //open listener on local endpoint
	~listener();

	/*
	accept:
		Blocks until connection can be accepted. Returns shared_ptr to nstream or
		empty shared_ptr if error. If the socket is non-blocking then an emtpy
		shared_ptr is returned if there are no more incoming connections.
		Precondition: is_open() = true.
	close:
		Close the socket.
	is_open:
		Returns true if listener listening.
	open:
		Start listener listening. The endpoint has a different purpose than with
		nstream. With a nstream we are connect to a endpoint on another host.
		With a listener we are the endpoint.
		Precondition: E must be tcp endpoint.
		Example:
		Only accept connections from localhost. Choose random port to listen on.
			network::get_endpoint("localhost", "0", network::tcp);
		Accept connections on all interfaces. Use port 1234.
			network::get_endpoint("", "1234", network::tcp);
	port:
		Returns port of listener on localhost, or empty string if not listening or
		error.
		Postcondition: If error socket is closed.
	set_non_blocking:
		Set the socket to be non-blocking.
	socket:
		Returns socket file descriptor, or -1 if not listening.
	*/
	boost::shared_ptr<nstream> accept();
	void close();
	bool is_open();
	void open(const endpoint & E);
	std::string port();
	void set_non_blocking();
	int socket();

private:
	//-1 if not open, or >= 0 if open
	int socket_FD;
};
}
#endif
