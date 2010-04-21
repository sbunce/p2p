//custom
#include "gui.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <CLI_args.hpp>
#include <gtkmm.h>

//std
#include <csignal>

boost::shared_ptr<Gtk::Main> gui_main;
boost::once_flag once_flag = BOOST_ONCE_INIT;

void quit_prog()
{
	gui_main->quit();
}

void signal_handler(int sig)
{
	signal(sig, signal_handler);
	boost::call_once(once_flag, &quit_prog);
}

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);
	gui_main = boost::shared_ptr<Gtk::Main>(new Gtk::Main(argc, argv));

	//register signal handlers
	signal(SIGINT, signal_handler);

	//test server functionality, run multiple servers out of same directory
	std::string str;
	if(CLI_Args.string("--test", str)){
		std::string port = str.substr(0, str.find_first_of(':'));
		std::string program_directory = str.substr(str.find_first_of(':')+1);
		p2p::test(port, program_directory);
	}

	gui GUI;
	gui_main->run(GUI);
}
