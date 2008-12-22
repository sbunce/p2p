//custom
#include "client_server_bridge.h"
#include "global.h"
#include "server.h"

int main(int argc, char ** argv)
{
	client_server_bridge::unblock_server_index();
	server Server;
	while(true){
		portable_sleep::ms(1000);
	}
	return 0;
}
