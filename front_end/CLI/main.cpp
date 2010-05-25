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

	boost::optional<std::string> program_dir = Opt_Parse.string("program_dir");
	if(program_dir){
		p2p::set_program_dir(*program_dir);
	}
	boost::optional<std::string> db_file_name = Opt_Parse.string("db_file_name");
	if(db_file_name){
		p2p::set_db_file_name(*db_file_name);
	}

	p2p P2P;

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope
}
