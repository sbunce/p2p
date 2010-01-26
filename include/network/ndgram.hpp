#ifndef H_NDGRAM
#define H_NDGRAM

//custom
#include "protocol.hpp"
#include "system_include.hpp"

namespace network{
class ndgram : private boost::noncopyable
{
public:
	ndgram():
		socket_FD(-1),
		_error(0)
	{
		open();
	}

	//create ndgram for sending/receiving
	ndgram(const endpoint & E):
		socket_FD(-1),
		_error(0)
	{
		open(E);
	}

	~ndgram()
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

	//returns true if we can send/recv data
	bool is_open() const
	{
		return socket_FD != -1;
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

	//open for sending
	void open()
	{
		close();
		if((socket_FD = ::socket(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP)) == -1){
			LOGGER << errno;
			_error = errno;
			close();
		}
	}

	//open for sending/receiving, endpoint is local
	void open(const endpoint & E)
	{
		assert(E.type() == udp);
		close();
		if((socket_FD = ::socket(E.ai.ai_family, E.ai.ai_socktype, E.ai.ai_protocol)) == -1){
			LOGGER << errno;
			_error = errno;
			close();
		}
		if(::bind(socket_FD, E.ai.ai_addr, E.ai.ai_addrlen) == -1){
			LOGGER << errno;
			_error = errno;
			close();
			return;
		}
	}

private:
	int socket_FD; //-1 if not connected, or >= 0 if connected
	int _error;    //most recent error, or 0 if no error

	//endpoint of host that sent most recent data
	boost::shared_ptr<endpoint> E;
};
}//end of namespace network
#endif
