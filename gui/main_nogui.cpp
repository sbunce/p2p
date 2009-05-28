//p2p
#include <p2p.hpp>

//include
#include <portable_sleep.hpp>

//std
#include <csignal>

namespace
{
	//volatile needed to stop optimizer from check-once behavior on while(!quit)
	volatile sig_atomic_t terminate_program = 0;

	void signal_handler(int sig)
	{
		signal(sig, signal_handler);
		terminate_program = 1;
	}
}//end of unnamed namespace

int main()
{
	//register signal handlers
	signal(SIGINT, signal_handler);

	p2p P2P;
	while(!terminate_program){
		portable_sleep::ms(1000);
	}
}
