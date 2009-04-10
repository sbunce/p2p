//custom
#include "network.hpp"

//include
#include <convert.hpp>

//std
#include <iostream>

void connect_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff, network::direction Direction)
{
	recv_buff.reserve(1024*8);
}

bool recv_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff)
{
	if(recv_buff.find("\n\r")){
		recv_buff.clear();
		send_buff.append((unsigned char *)"ABC", 3);
	}
	return true;
}

bool send_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff)
{
	if(send_buff.empty()){
		return false;
	}else{
		return true;
	}
}

void disconnect_call_back(int socket_FD)
{
	LOGGER << socket_FD;
}

//simulates GUI thread
void monitor_loop(network & Network)
{
	while(true){
		std::cout << "upload:      " << convert::size_SI(Network.current_upload_rate()) << "\n";
		std::cout << "download:    " << convert::size_SI(Network.current_download_rate()) << "\n";
		std::cout << "connections: " << Network.connections() << "\n";
		sleep(1);
	}
}

int main()
{
	network Network(
		&connect_call_back,
		&recv_call_back,
		&send_call_back,
		&disconnect_call_back,
		4,
		4,
		6969
	);
	//Network.add_connection("209.86.171.1", 6968);
	Network.set_max_upload_rate(1024*1024*5);
	monitor_loop(Network);
}
