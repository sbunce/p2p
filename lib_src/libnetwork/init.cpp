#include "system_include.hpp"
#include "init.hpp"

network::init::init()
{
	start();
}

network::init::~init()
{
	stop();
}

void network::init::start()
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

void network::init::stop()
{
	#ifdef _WIN32
	WSACleanup();
	#endif
}
