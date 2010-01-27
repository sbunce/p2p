#ifndef H_NDGRAM
#define H_NDGRAM

//custom
#include "protocol.hpp"
#include "system_include.hpp"

namespace network{
class ndgram : private boost::noncopyable
{
	//max we attempt to send/recv in one go
	static const int MTU = 16384;

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
		std::set<endpoint> E = get_endpoint("", "0", udp);
		assert(!E.empty());
		if((socket_FD = ::socket(E.begin()->ai.ai_family, E.begin()->ai.ai_socktype,
			E.begin()->ai.ai_protocol)) == -1)
		{
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

	/*
	Receive data. Returns number of bytes received or 0 if the host disconnected.
	E is set to the endpoint of the host that sent the data. E may be empty if
	error.
	*/
	int recv(network::buffer & buf, boost::shared_ptr<endpoint> & E)
	{
		E = boost::shared_ptr<endpoint>();
		addrinfo ai;
		sockaddr_storage sas;
		ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
		ai.ai_addrlen = sizeof(sockaddr_storage);
		buf.tail_reserve(MTU);
		int n_bytes = ::recvfrom(socket_FD, reinterpret_cast<char *>(buf.tail_start()),
			buf.tail_size(), 0, ai.ai_addr, &ai.ai_addrlen);
		if(n_bytes == -1){
			LOGGER << errno;
			_error = errno;
		}else if(n_bytes == 0){
			close();
			buf.tail_reserve(0);
		}else{
			E = boost::shared_ptr<endpoint>(new endpoint(&ai));
			buf.tail_resize(n_bytes);
		}
		return n_bytes;
	}

	/*
	Writes bytes from buffer. Returns the number of bytes sent or 0 if the host
	disconnected. The sent bytes are erased from the buffer.
	*/
	int send(network::buffer & buf, const endpoint & E)
	{
		int n_bytes = ::sendto(socket_FD, reinterpret_cast<char *>(buf.data()),
			buf.size(), 0, E.ai.ai_addr, E.ai.ai_addrlen);
		if(n_bytes == -1){
			LOGGER << errno;
			_error = errno;
		}else if(n_bytes == 0){
			close();
		}else{
			buf.erase(0, n_bytes);
		}
		return n_bytes;
	}

private:
	int socket_FD; //-1 if not connected, or >= 0 if connected
	int _error;    //most recent error, or 0 if no error
};
}//end of namespace network
#endif
