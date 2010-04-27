//custom
#include "http.hpp"

//include
#include <boost/thread.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <csignal>

//needed to priveledge drop
#ifndef __WIN32
	//posix
	#include <sys/types.h> 
	#include <unistd.h>
#endif

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

	if(argc == 1){
		std::cout
			<< "usage: " << argv[0] << " <options> <web_root>\n"
			<< "\n"
			<< "options:\n"
			<< " -l         Listen on localhost only.\n"
			<< " -uid=<uid> User ID to priviledge drop to.\n"
			<< " -gid=<gid> Group ID to priviledge drop to.\n"
			<< " -p=<port>  Port number 0 to 65535 (default random).\n"
			<< " -r=<rate>  Max upload rate (B/s) (default unlimited).\n"
		;
		exit(1);
	}

	//optional localhost only
	bool localhost_only = CLI_Args.flag("-l");

	//optional pid and gid
	unsigned uid = 0, gid = 0;
	CLI_Args.uint("-uid", uid);
	CLI_Args.uint("-gid", gid);

	//optional port
	std::string port = "0";
	CLI_Args.string("-p", port);

	//optional max upload rate
	unsigned max_upload_rate = 0;
	CLI_Args.uint("-r", max_upload_rate);

	http HTTP(CLI_Args[CLI_Args.argc() - 1]);

	//priviledge drop before server started
	#ifndef __WIN32
	if(uid != 0){
		if(setuid(uid) == -1){
			LOG << errno;
		}
	}
	if(gid != 0){
		if(setgid(gid) == -1){
			LOG << errno;
		}
	}
	#endif

	HTTP.start(port);

	LOG << "listen on " << HTTP.port();
	HTTP.set_max_upload_rate(max_upload_rate);

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope
}
