#include "init.hpp"

//include
#include <portable.hpp>

net::init::init()
{
	start();
}

net::init::~init()
{
	stop();
}

void net::init::start()
{
	#ifdef _WIN32
	WORD wsock_ver = MAKEWORD(2,2);
	WSADATA wsock_data;
	int err;
	if((err = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOG << "winsock error " << err;
		exit(1);
	}
	#endif
}

void net::init::stop()
{
	#ifdef _WIN32
	WSACleanup();
	#endif
}
