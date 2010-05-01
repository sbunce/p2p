#include <network/start.hpp>

network::start::start()
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

network::start::~start()
{
	#ifdef _WIN32
	WSACleanup();
	#endif
}
