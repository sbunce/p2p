#include "init.hpp"
#include <network/network.hpp>

network::ndgram::ndgram():
	socket_FD(-1)
{
	network::init::start();
	std::set<endpoint> E = get_endpoint("", "0");
	assert(!E.empty());
	if((socket_FD = ::socket(E.begin()->ai.ai_family, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		LOG << strerror(errno);
		close();
	}
}

network::ndgram::ndgram(const endpoint & E):
	socket_FD(-1)
{
	network::init::start();
	open(E);
}

network::ndgram::~ndgram()
{
	close();
	network::init::stop();
}

void network::ndgram::close()
{
	if(socket_FD != -1){
		if(::close(socket_FD) == -1){
			LOG << strerror(errno);
		}
		socket_FD = -1;
	}
}

bool network::ndgram::is_open() const
{
	return socket_FD != -1;
}

std::string network::ndgram::local_IP()
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

std::string network::ndgram::local_port()
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

void network::ndgram::open(const endpoint & E)
{
	close();
	if((socket_FD = ::socket(E.ai.ai_family, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		LOG << strerror(errno);
		close();
	}
	if(::bind(socket_FD, E.ai.ai_addr, E.ai.ai_addrlen) == -1){
		LOG << strerror(errno);
		close();
		return;
	}
}

int network::ndgram::recv(network::buffer & buf, boost::shared_ptr<endpoint> & E)
{
	E = boost::shared_ptr<endpoint>();
	addrinfo ai;
	sockaddr_storage sas;
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
	ai.ai_addrlen = sizeof(sockaddr_storage);
	buf.tail_reserve(MTU);
	int n_bytes = ::recvfrom(socket_FD, reinterpret_cast<char *>(buf.tail_start()),
		buf.tail_size(), 0, ai.ai_addr, reinterpret_cast<socklen_t *>(&ai.ai_addrlen));
	if(n_bytes == -1){
		LOG << strerror(errno);
	}else if(n_bytes == 0){
		close();
		buf.tail_reserve(0);
	}else{
		E = boost::shared_ptr<endpoint>(new endpoint(&ai));
		buf.tail_resize(n_bytes);
	}
	return n_bytes;
}

int network::ndgram::send(network::buffer & buf, const endpoint & E)
{
	int n_bytes = ::sendto(socket_FD, reinterpret_cast<char *>(buf.data()),
		buf.size(), 0, E.ai.ai_addr, E.ai.ai_addrlen);
	if(n_bytes == -1){
		LOG << strerror(errno);
	}else if(n_bytes == 0){
		close();
	}else{
		buf.erase(0, n_bytes);
	}
	return n_bytes;
}

int network::ndgram::socket()
{
	return socket_FD;
}
