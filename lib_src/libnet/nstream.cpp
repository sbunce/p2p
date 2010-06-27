#include "init.hpp"
#include "addr_impl.hpp"
#include <net/net.hpp>

net::nstream::nstream()
{

}

net::nstream::nstream(const endpoint & ep)
{
	open(ep);
}

net::nstream::nstream(const int socket_FD_in)
{
	socket_FD = socket_FD_in;
}

bool net::nstream::is_open_async()
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
			return false;
		}
	}else{
		//error getting option
		LOG << strerror(errno);
		return false;
	}
}

std::string net::nstream::local_IP()
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

std::string net::nstream::local_port()
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

void net::nstream::open(const endpoint & ep)
{
	close();
	if((socket_FD = ::socket(ep.AI->ai.ai_addr->sa_family, SOCK_STREAM, IPPROTO_TCP)) == -1){
		LOG << strerror(errno);
		close();
	}
	if(::connect(socket_FD, ep.AI->ai.ai_addr, ep.AI->ai.ai_addrlen) != 0){
		LOG << strerror(errno);
		close();
	}
}

void net::nstream::open_async(const endpoint & ep)
{
	close();
	if((socket_FD = ::socket(ep.AI->ai.ai_addr->sa_family, SOCK_STREAM, IPPROTO_TCP)) == -1){
		LOG << strerror(errno);
		return;
	}
	if(!set_non_blocking(true)){
		LOG << strerror(errno);
		close();
		return;
	}
	/*
	FreeBSD:
		If connect returns 0 the socket MAY or MAY NOT be connected.
	Windows/Linux:
		If connect returns 0 the socket connected immediately.

	Because of the inconsistency above we check for writeability because it
	works on FreeBSD, Windows, and Linux.
	*/
	if(::connect(socket_FD, ep.AI->ai.ai_addr, ep.AI->ai.ai_addrlen) != 0){
		/*
		Socket in progress of connecting.
		Note: Windows will sometimes return SOCKET_ERROR (-1) from connect and
			errno will be set to 0. In this case we can ignore the error.
		*/
		if(errno != EINPROGRESS && errno != EWOULDBLOCK && errno != 0){
			LOG << strerror(errno);
			exit(1);
		}
	}
}

int net::nstream::recv(buffer & buf, const int max_transfer)
{
	assert(max_transfer > 0);
	if(max_transfer > MTU){
		buf.tail_reserve(MTU);
	}else{
		buf.tail_reserve(max_transfer);
	}
	if(socket_FD == -1){
		//socket previously disconnected, errno might not be valid here
		return 0;
	}else{
		int n_bytes = ::recv(socket_FD, reinterpret_cast<char *>(buf.tail_start()),
			buf.tail_size(), MSG_NOSIGNAL);
		if(n_bytes == -1){
			LOG << strerror(errno);
			close();
		}else if(n_bytes == 0){
			close();
			buf.tail_reserve(0);
		}else{
			buf.tail_resize(n_bytes);
		}
		return n_bytes;
	}
}

std::string net::nstream::remote_IP()
{
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getpeername(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
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

std::string net::nstream::remote_port()
{
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getpeername(socket_FD, reinterpret_cast<sockaddr *>(&addr), &addrlen) == -1){
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

int net::nstream::send(buffer & buf, int max_transfer)
{
	assert(max_transfer > 0);
	if(max_transfer > buf.size()){
		max_transfer = buf.size();
	}
	if(max_transfer > MTU){
		max_transfer = MTU;
	}
	if(socket_FD == -1){
		//socket previously disconnected, errno might not be valid here
		return 0;
	}else{
		int n_bytes = ::send(socket_FD, reinterpret_cast<char *>(buf.data()),
			max_transfer, MSG_NOSIGNAL);
		if(n_bytes == -1){
			LOG << strerror(errno);
			close();
		}else if(n_bytes == 0){
			close();
		}else{
			buf.erase(0, n_bytes);
		}
		return n_bytes;
	}
}
