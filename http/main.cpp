//include
#include <boost/thread.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <csignal>

boost::mutex terminate_mutex;
boost::condition_variable_any terminate_cond;

//volatile needed to stop while(!terminate_program) -> while(true) optimization
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

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);

	//register signal handlers
	signal(SIGINT, signal_handler);

	std::string web_root;
	if(!CLI_Args.string("--web_root", web_root)){
		LOGGER << "web root not specified";
		exit(1);
	}

	network::http::server HTTP(web_root);
	LOGGER << "listening on " << HTTP.port();

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope
}
