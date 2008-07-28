//custom
#include "server.h"

int main(int argc, char * argv[])
{
	server Server;
	while(true){
		#ifdef WIN32
		Sleep(0);
		#else
		usleep(1);
		#endif
	}
	return 0;
}

