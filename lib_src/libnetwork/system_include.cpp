#include <network/system_include.hpp>

void network::start()
{
	#ifdef _WIN32
	WORD wsock_ver = MAKEWORD(2,2);
	WSADATA wsock_data;
	int err;
	if((err = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOGGER(logger::fatal) << "winsock error " << err;
		exit(1);
	}
	#endif
}

void network::stop()
{
	#ifdef _WIN32
	WSACleanup();
	#endif
}
