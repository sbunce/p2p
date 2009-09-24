//include
#include <boost/thread.hpp>
#include <p2p.hpp>

//standard
#include <csignal>

namespace
{
boost::mutex terminate_mutex;
boost::condition_variable_any terminate_cond;

//volatile needed to stop optimizer turning while(!quit) in to while(true)
volatile sig_atomic_t terminate_program = 0;

void signal_handler(int sig)
{
	signal(sig, signal_handler);

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	terminate_program = 1;
	terminate_cond.notify_one();
	}//end lock scope
}
}//end of unnamed namespace

int main()
{
	//register signal handlers
	signal(SIGINT, signal_handler);

	p2p P2P;

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope
}
