//include
#include <p2p/p2p.hpp>
#include <portable_sleep.hpp>

//standard
#include <csignal>

namespace
{
	//volatile needed to stop optimizer turning while(!quit) in to while(true)
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
