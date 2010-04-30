//custom
#include "http.hpp"

//include
#include <boost/thread.hpp>
#include <logger.hpp>
#include <network/network.hpp>
#include <opt_parse.hpp>

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
	opt_parse Opt_Parse(argc, argv);

	//register signal handlers
	signal(SIGINT, signal_handler);

	std::stringstream help;
	help
		<< "\n"
		<< "usage: " << argv[0] << " <options>\n"
		<< "\n"
		<< "required options:\n"
		<< " --web_root <path> Path to web root.\n"
		<< "\n"
		<< "options:\n"
		<< " -l                Listen on localhost only.\n"
		#ifndef __WIN32
		<< " --uid <uid>       User ID to priviledge drop to.\n"
		<< " --gid <gid>       Group ID to priviledge drop to.\n"
		#endif
		<< " --port <port>     Port number 0 to 65535 (default random).\n"
		<< " --rate <rate>     Max upload rate (B/s) (default unlimited).\n"
		<< " --help            Show this dialogue.\n"
		<< "\n"
	;

	//parse options
	if(Opt_Parse.flag("help")){
		std::cout << help.str();
		return 0;
	}
	boost::optional<std::string> web_root = Opt_Parse.string("web_root");
	if(!web_root){
		std::cout
			<< "error, web root not specified\n"
			<< "--help for usage information\n";
		return 1;
	}
	bool localhost_only = Opt_Parse.flag("l");
	#ifndef __WIN32
	boost::optional<uid_t> uid = Opt_Parse.lexical_cast<uid_t>("uid");
	boost::optional<gid_t> gid = Opt_Parse.lexical_cast<gid_t>("gid");
	#endif
	boost::optional<std::string> port = Opt_Parse.string("port");
	boost::optional<unsigned> rate = Opt_Parse.lexical_cast<unsigned>("rate");
	if(Opt_Parse.unparsed()){
		std::cout << "--help for usage information\n";
		return 1;
	}

	//start HTTP server
	http HTTP(*web_root, port ? *port : "0", localhost_only);
	HTTP.start_0();
	//priviledge drop before incoming connections allowed
	#ifndef __WIN32
	if(uid){
		if(setuid(*uid) == -1){
			LOG << errno;
		}
	}
	if(gid){
		if(setgid(*gid) == -1){
			LOG << errno;
		}
	}
	#endif
	if(rate){
		HTTP.set_max_upload_rate(*rate);
	}
	//allow incoming connections
	HTTP.start_1();
	LOG << "listen on " << HTTP.listen_port();

	//wait for server to terminate
	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope
}
