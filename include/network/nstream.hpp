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
class nstream : boost::noncopyable
{
	//max we attempt to send/recv in one go
	static const int MTU = 16384;

public:

	nstream():
		socket_FD(-1),
		_error(0)
	{

	}

	//sync connect to endpoint
	nstream(const endpoint & E):
		socket_FD(-1),
		_error(0)
	{
		open(E);
	}

	//create nstream out of already created socket
	nstream(const int socket_FD_in):
		socket_FD(socket_FD_in),
		_error(0)
	{

	}

	~nstream()
	{
		close();
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

	//returns errno that resulted from a send/recv error, or 0 if no error
	int error() const
	{
		return _error;
	}

	/*
	Returns true if connected.
	WARNING: This function returns a false positive if a socket is asynchronously
		connecting.
	*/
	bool is_open() const
	{
		return socket_FD != -1;
	}

	/*
	After open_async we must wait for the socket to become writeable. When it is
	we can call this to see if the connection succeeded.
	WARNING: If this is called before the socket becomes writeable we can have a
		false negative.
	*/
	bool is_open_async()
	{
		int opt_val = 1;
		socklen_t opt_len = sizeof(opt_val);
		if(getsockopt(socket_FD, SOL_SOCKET, SO_ERROR,
			reinterpret_cast<char *>(&opt_val), &opt_len) == 0)
		{
			//we got option
			if(opt_val == 0){
				return true;
			}else{
				LOGGER << errno;
				_error = errno;
				return false;
			}
		}else{
			//error getting option
			LOGGER << errno;
			_error = errno;
			return false;
		}
	}

	//returns local port, or empty string if error
	std::string local_IP()
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
		char buf[INET6_ADDRSTRLEN];
		if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, buf,
			sizeof(buf), NULL, 0, NI_NUMERICHOST) == -1)
		{
			LOGGER << errno;
			_error = errno;
			close();
			return "";
		}
		return buf;
	}

	//returns local port, or empty string if error
	std::string local_port()
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

	//opens connection
	void open(const endpoint & E)
	{
		assert(E.type() == tcp);
		close();
		if((socket_FD = ::socket(E.ai.ai_family, E.ai.ai_socktype, E.ai.ai_protocol)) == -1){
			LOGGER << errno;
			_error = errno;
			close();
		}
		if(::connect(socket_FD, E.ai.ai_addr, E.ai.ai_addrlen) != 0){
			LOGGER << errno;
			_error = errno;
			close();
		}
	}

	/*
	Start async connection. Returns true if connected, or false if failed to
	connect or in progress of connecting. When the socket connects or fails to
	connect it will become writeable.
	*/
	bool open_async(const endpoint & E)
	{
		close();
		if((socket_FD = ::socket(E.ai.ai_family, E.ai.ai_socktype,
			E.ai.ai_protocol)) == -1)
		{
			LOGGER << errno;
			exit(1);
		}
		set_non_blocking();
		if(::connect(socket_FD, E.ai.ai_addr, E.ai.ai_addrlen) == 0){
			return true;
		}else{
			//socket in progress of connecting
			if(errno != EINPROGRESS && errno != EWOULDBLOCK){
				LOGGER << errno;
				exit(1);
			}
			return false;
		}
	}

	/*
	Reads bytes in to buffer. Returns the number of bytes read or 0 if the host
	disconnected. Returns -1 on error.
	*/
	int recv(buffer & B, const int max_transfer = MTU)
	{
		assert(max_transfer > 0);
		if(max_transfer > MTU){
			B.tail_reserve(MTU);
		}else{
			B.tail_reserve(max_transfer);
		}

		if(socket_FD == -1){
			//socket previously disconnected, errno might not be valid here
			return 0;
		}else{
			int n_bytes = ::recv(socket_FD, reinterpret_cast<char *>(B.tail_start()),
				B.tail_size(), MSG_NOSIGNAL);
			if(n_bytes == -1){
				LOGGER << errno;
				_error = errno;
			}else if(n_bytes == 0){
				close();
				B.tail_reserve(0);
			}else{
				B.tail_resize(n_bytes);
			}
			return n_bytes;
		}
	}

	//returns local port, or empty string if error
	std::string remote_IP()
	{
		sockaddr_storage addr;
		socklen_t addrlen = sizeof(addr);
		if(getpeername(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
			LOGGER << errno;
			_error = errno;
			close();
			return "";
		}
		char buf[INET6_ADDRSTRLEN];
		if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, buf,
			sizeof(buf), NULL, 0, NI_NUMERICHOST) == -1)
		{
			LOGGER << errno;
			_error = errno;
			close();
			return "";
		}
		return buf;
	}

	//returns local port, or empty string if error
	std::string remote_port()
	{
		sockaddr_storage addr;
		socklen_t addrlen = sizeof(addr);
		if(getpeername(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
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

	/*
	Writes bytes from buffer. Returns the number of bytes sent or 0 if the host
	disconnected. The sent bytes are erased from the buffer.
	*/
	int send(buffer & B, int max_transfer = MTU)
	{
		assert(max_transfer > 0);
		if(max_transfer > B.size()){
			max_transfer = B.size();
		}
		if(max_transfer > MTU){
			max_transfer = MTU;
		}

		if(socket_FD == -1){
			//socket previously disconnected, errno might not be valid here
			return 0;
		}else{
			int n_bytes = ::send(socket_FD, reinterpret_cast<char *>(B.data()),
				max_transfer, MSG_NOSIGNAL);
			if(n_bytes == -1){
				_error = errno;
			}else if(n_bytes == 0){
				close();
			}else{
				B.erase(0, n_bytes);
			}
			return n_bytes;
		}
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

	//returns socket file descriptor, or -1 if disconnected
	int socket()
	{
		return socket_FD;
	}

private:
	int socket_FD; //-1 if not connected, or >= 0 if connected
	int _error;    //most recent error, or 0 if no error
};
}
#endif
