//include
#include <boost/thread.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
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
	CLI_args CLI_Args(argc, argv);

	//register signal handlers
	signal(SIGINT, signal_handler);

	//test server functionality, run multiple servers out of same directory
	std::string str;
	if(CLI_Args.string("--test", str)){
		std::string port = str.substr(0, str.find_first_of(':'));
		std::string program_directory = str.substr(str.find_first_of(':')+1);
		if(!program_directory.empty() && program_directory[program_directory.size()-1] != '/'){
			//make sure path ends with slash
			program_directory += '/';
		}
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
