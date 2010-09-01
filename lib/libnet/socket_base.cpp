#include <net/socket_base.hpp>

net::socket_base::socket_base():
	socket_FD(-1)
{
	net::init::start();
}

net::socket_base::~socket_base()
{
	close();
	net::init::stop();
}

void net::socket_base::close()
{
	if(socket_FD != -1){
		if(::close_socket(socket_FD) == -1){
			LOG << strerror(errno);
		}
		socket_FD = -1;
	}
}

bool net::socket_base::is_open() const
{
	return socket_FD != -1;
}

boost::optional<net::endpoint> net::socket_base::local_ep()
{
	//IP
	if(socket_FD == -1){
		return boost::optional<net::endpoint>();
	}
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getsockname(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}
	char IP[INET6_ADDRSTRLEN];
	if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, IP, sizeof(IP),
		NULL, 0, NI_NUMERICHOST) == -1)
	{
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}

	//port
	char port[6];
	if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, NULL, 0, port,
		sizeof(port), NI_NUMERICSERV) == -1)
	{
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}

	std::set<endpoint> E = get_endpoint(IP, port);
	if(E.empty()){
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}
	return *E.begin();
}

boost::optional<net::endpoint> net::socket_base::remote_ep()
{
	//IP
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getpeername(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}
	char IP[INET6_ADDRSTRLEN];
	if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, IP, sizeof(IP),
		NULL, 0, NI_NUMERICHOST) == -1)
	{
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}

	//port
	char port[6];
	if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, NULL, 0, port,
		sizeof(port), NI_NUMERICSERV) == -1)
	{
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}

	std::set<endpoint> E = get_endpoint(IP, port);
	if(E.empty()){
		LOG << strerror(errno);
		close();
		return boost::optional<net::endpoint>();
	}
	return *E.begin();
}

bool net::socket_base::set_non_blocking(const bool val)
{
	if(socket_FD != -1){
		if(val){
			//set non blocking
			#ifdef _WIN32
			u_long mode = 1;
			if(ioctlsocket(socket_FD, FIONBIO, &mode) == -1){
				LOG << strerror(errno);
				close();
				return false;
			}
			#else
			int flags;
			if((flags = fcntl(socket_FD, F_GETFL, 0)) == -1){
				LOG << strerror(errno);
				close();
				return false;
			}
			if(fcntl(socket_FD, F_SETFL, flags | O_NONBLOCK) == -1){
				LOG << strerror(errno);
				close();
				return false;
			}
			#endif
		}else{
			//set blocking
			#ifdef _WIN32
			u_long mode = 0;
			if(ioctlsocket(socket_FD, FIONBIO, &mode) == -1){
				LOG << strerror(errno);
				close();
				return false;
			}
			#else
			int flags;
			if((flags = fcntl(socket_FD, F_GETFL, 0)) == -1){
				LOG << strerror(errno);
				close();
				return false;
			}
			if(fcntl(socket_FD, F_SETFL, flags & ~O_NONBLOCK) == -1){
				LOG << strerror(errno);
				close();
				return false;
			}
			#endif
		}
	}
	return true;
}

int net::socket_base::socket()
{
	return socket_FD;
}
