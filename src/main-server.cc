//std
#include <ctime>
#include <iostream>

#include "server.h"

int main(int argc, char* argv[])
{
	std::cout << "server started\n";

	server Server;
	Server.start();

	//run forever
	while(true){
		sleep(1);
	}

	return 0;
}
