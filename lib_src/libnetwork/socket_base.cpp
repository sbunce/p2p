#include "init.hpp"
#include "system_include.hpp"
#include <network/network.hpp>

network::socket_base::socket_base():
	socket_FD(-1)
{
	network::init::start();
}

network::socket_base::~socket_base()
{
	close();
	network::init::stop();
}

void network::socket_base::close()
{
	if(socket_FD != -1){
		if(::close(socket_FD) == -1){
			LOG << strerror(errno);
		}
		socket_FD = -1;
	}
}

bool network::socket_base::is_open() const
{
	return socket_FD != -1;
}

bool network::socket_base::set_non_blocking(const bool val)
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

int network::socket_base::socket()
{
	return socket_FD;
}
