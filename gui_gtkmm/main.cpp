//custom
#include "gui.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <CLI_args.hpp>
#include <gtkmm.h>

//std
#include <csignal>

Gtk::Main * Main_ptr;

void signal_handler(int sig)
{
	signal(sig, signal_handler);
	Main_ptr->quit();
}

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);

	Gtk::Main Main(argc, argv);
	Main_ptr = &Main;

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
	Main.run(GUI);
}
