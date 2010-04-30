//include
#include <boost/thread.hpp>
#include <logger.hpp>
#include <opt_parse.hpp>
#include <p2p.hpp>

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
	opt_parse Opt_Parse(argc, argv);

	//register signal handlers
	signal(SIGINT, signal_handler);

	//test server functionality, run multiple servers out of same directory
	boost::optional<std::string> test = Opt_Parse.string("--test");
	if(test){
		std::string port = test->substr(0, test->find_first_of(':'));
		std::string program_directory = test->substr(test->find_first_of(':')+1);
		p2p::test(port, program_directory);
	}

	p2p P2P;

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope
}
