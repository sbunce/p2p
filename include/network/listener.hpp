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

	listener():
		socket_FD(-1),
		_error(0)
	{

	}

	listener(const endpoint & E):
		socket_FD(-1),
		_error(0)
	{
		open(E);
	}

	~listener()
	{
		close();
	}

	/*
	Blocks until connection can be accepted.
	Returns shared_ptr to nstream or empty shared_ptr if error. If the socket is
		non-blocking then an emtpy shared_ptr is returned if there are no more
		incoming connections.
	Precondition: is_open() = true.
	*/
	boost::shared_ptr<nstream> accept()
	{
		assert(is_open());
		addrinfo ai;
		sockaddr_storage sas;
		ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
		ai.ai_addrlen = sizeof(sockaddr_storage);
		int new_socket = ::accept(socket_FD, ai.ai_addr,
			reinterpret_cast<socklen_t *>(&ai.ai_addrlen));
		if(new_socket == -1){
			_error = errno;
			return boost::shared_ptr<nstream>();
		}
		boost::shared_ptr<nstream> N(new nstream(new_socket));
		E = boost::shared_ptr<endpoint>(new endpoint(&ai));
		return N;
	}

	/*
	Returns endpoint for latest accepted connection.
	Precondition: At least one connection must have been accepted.
	*/
	endpoint & accept_endpoint()
	{
		assert(E);
		return *E;
	}

	//close the socket
	void close()
	{
		if(socket_FD != -1){
			if(::close(socket_FD) == -1){
				LOGGER << errno;
				_error = errno;
			}
			socket_FD = -1;
		}
	}

	//returns errno for latest error
	int error() const
	{
		return _error;
	}

	//returns true if listener listening
	bool is_open()
	{
		return socket_FD != -1;
	}

	/*
	Start listener listening. The endpoint has a different purpose than with
	nstream. With a nstream we are connect to a endpoint on another host.
	With a listener we are the endpoint.
	Common Endpoints:

		Only accept connections from localhost. Choose random port to listen on.
		network::get_endpoint("localhost", "0", network::tcp);

		Accept connections on all interfaces. Use port 1234.
		network::get_endpoint("", "1234", network::tcp);

	Precondition: E must be tcp endpoint.
	*/
	void open(const endpoint & E)
	{
		assert(E.type() == tcp);
		close();
		if((socket_FD = ::socket(E.ai.ai_family, E.ai.ai_socktype,
			E.ai.ai_protocol)) == -1)
		{
			LOGGER << errno;
			_error = errno;
			close();
			return;
		}
		/*
		This takes care of "address already in use" errors that can happen when
		trying to bind to a port that wasn't properly closed and hasn't timed
		out.
		*/
		int optval = 1;
		socklen_t optlen = sizeof(optval);
		if(::setsockopt(socket_FD, SOL_SOCKET, SO_REUSEADDR,
			reinterpret_cast<char *>(&optval), optlen) == -1)
		{
			LOGGER << errno;
			_error = errno;
			close();
			return;
		}
		if(::bind(socket_FD, E.ai.ai_addr, E.ai.ai_addrlen) == -1){
			LOGGER << errno;
			_error = errno;
			close();
			return;
		}
		int backlog = 512; //max pending accepts
		if(::listen(socket_FD, backlog) == -1){
			LOGGER << errno;
			_error = errno;
			close();
			return;
		}
	}

	/*
	Returns port of listener on localhost, or empty string if not listening or
	error.
	*/
	std::string port()
	{
		if(socket_FD == -1){
			return "";
		}
		sockaddr_storage addr;
		socklen_t addrlen = sizeof(addr);
		if(getsockname(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
			LOGGER << errno;
			_error = errno;
			close();
			return "";
		}
		char buf[6];
		if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, NULL, 0, buf,
			sizeof(buf), NI_NUMERICSERV) == -1)
		{
			LOGGER << errno;
			_error = errno;
			close();
			return "";
		}
		return buf;
	}

	void set_non_blocking()
	{
		if(socket_FD != -1){
			#ifdef _WIN32
			u_long mode = 1;
			if(ioctlsocket(socket_FD, FIONBIO, &mode) == -1){
				LOGGER << errno;
				_error = errno;
				close();
			}
			#else
			if(fcntl(socket_FD, F_SETFL, O_NONBLOCK) == -1){
				LOGGER << errno;
				_error = errno;
				close();
			}
			#endif
		}
	}

	//returns socket file descriptor, or -1 if not listening
	int socket()
	{
		return socket_FD;
	}

private:
	//-1 when not listening and >= 0 when listening
	int socket_FD;

	//holds most recent error
	int _error;

	//endpoint of most recently accepted connection (empty if no connection accepted)
	boost::shared_ptr<endpoint> E;
};
}
#endif
