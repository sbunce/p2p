#include "init.hpp"
#include "addr_impl.hpp"
#include <net/net.hpp>

net::ndgram::ndgram()
{
	std::set<endpoint> ep = get_endpoint("", "0");
	assert(!ep.empty());
	if((socket_FD = ::socket(ep.begin()->AI->ai.ai_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		LOG << strerror(errno);
		close();
	}
}

net::ndgram::ndgram(const endpoint & ep)
{
	open(ep);
}

std::string net::ndgram::local_IP()
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

std::string net::ndgram::local_port()
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

void net::ndgram::open(const endpoint & ep)
{
	close();
	if((socket_FD = ::socket(ep.AI->ai.ai_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		LOG << strerror(errno);
		close();
	}
	if(bind(socket_FD, ep.AI->ai.ai_addr, ep.AI->ai.ai_addrlen) == -1){
		LOG << strerror(errno);
		close();
		return;
	}
}

int net::ndgram::recv(net::buffer & buf, boost::shared_ptr<endpoint> & ep)
{
	ep.reset();
	addrinfo ai;
	sockaddr_storage sas;
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
	ai.ai_addrlen = sizeof(sockaddr_storage);
	buf.tail_reserve(MTU);
	int n_bytes = recvfrom(socket_FD, reinterpret_cast<char *>(buf.tail_start()),
		buf.tail_size(), 0, ai.ai_addr, reinterpret_cast<socklen_t *>(&ai.ai_addrlen));
	if(n_bytes == -1 || n_bytes == 0){
		LOG << strerror(errno);
		close();
		buf.tail_reserve(0);
	}else{
		boost::scoped_ptr<addr_impl> AI(new addr_impl(&ai));
		ep.reset(new endpoint(AI));
		buf.tail_resize(n_bytes);
	}
	return n_bytes;
}

int net::ndgram::send(net::buffer & buf, const endpoint & ep)
{
	int n_bytes = sendto(socket_FD, reinterpret_cast<char *>(buf.data()),
		buf.size(), 0, ep.AI->ai.ai_addr, ep.AI->ai.ai_addrlen);
	if(n_bytes == -1 || n_bytes == 0){
		LOG << strerror(errno);
		close();
	}else{
		buf.erase(0, n_bytes);
	}
	return n_bytes;
}
