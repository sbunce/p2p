#include <net/listener.hpp>

net::listener::listener()
{

}

net::listener::listener(const endpoint & ep)
{
	open(ep);
}

boost::shared_ptr<net::nstream> net::listener::accept()
{
	assert(is_open());
	addrinfo ai;
	sockaddr_storage sas;
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
	ai.ai_addrlen = sizeof(sockaddr_storage);
	int new_socket = ::accept(socket_FD, ai.ai_addr,
		reinterpret_cast<socklen_t *>(&ai.ai_addrlen));
	if(new_socket == -1){
		return boost::shared_ptr<nstream>();
	}
	boost::shared_ptr<nstream> N(new nstream(new_socket));
	return N;
}

boost::optional<net::endpoint> net::listener::local_ep()
{
	return _local_ep;
}

std::string net::listener::local_IP()
{
	if(socket_FD == -1){
		return "";
	}
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getsockname(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
		LOG << strerror(errno);
		close();
		return "";
	}
	char buf[INET6_ADDRSTRLEN];
	if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, buf,
		sizeof(buf), NULL, 0, NI_NUMERICHOST) == -1)
	{
		LOG << strerror(errno);
		close();
		return "";
	}
	return buf;
}

std::string net::listener::local_port()
{
	if(socket_FD == -1){
		return "";
	}
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getsockname(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
		LOG << strerror(errno);
		close();
		return "";
	}
	char buf[6];
	if(getnameinfo(reinterpret_cast<sockaddr *>(&addr), addrlen, NULL, 0, buf,
		sizeof(buf), NI_NUMERICSERV) == -1)
	{
		LOG << strerror(errno);
		close();
		return "";
	}
	return buf;
}

void net::listener::open(const endpoint & ep)
{
	close();
	if((socket_FD = ::socket(ep.ai.ai_family, SOCK_STREAM,
		ep.ai.ai_protocol)) == -1)
	{
		LOG << strerror(errno);
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
		LOG << strerror(errno);
		close();
		return;
	}
	if(::bind(socket_FD, ep.ai.ai_addr, ep.ai.ai_addrlen) == -1){
		LOG << strerror(errno);
		close();
		return;
	}
	int backlog = 512; //max pending accepts
	if(::listen(socket_FD, backlog) == -1){
		LOG << strerror(errno);
		close();
		return;
	}
	set_local_ep();
}

void net::listener::set_local_ep()
{
	//local endpoint
	std::string IP = local_IP();
	if(IP.empty()){
		return;
	}
	std::string port = local_port();
	if(port.empty()){
		return;
	}
	std::set<endpoint> E = get_endpoint(IP, port);
	if(E.empty()){
		LOG << strerror(errno);
		close();
		return;
	}
	_local_ep = *E.begin();
}
