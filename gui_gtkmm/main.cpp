//custom
#include "gui.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <gtkmm.h>
#include <opt_parse.hpp>

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
	opt_parse Opt_Parse(argc, argv);
	gui_main = boost::shared_ptr<Gtk::Main>(new Gtk::Main(argc, argv));

	//register signal handlers
	signal(SIGINT, signal_handler);

	//test server functionality, run multiple servers out of same directory
	boost::optional<std::string> test = Opt_Parse.string("test");
	if(test){
		std::string port = test->substr(0, test->find_first_of(':'));
		std::string program_directory = test->substr(test->find_first_of(':')+1);
		p2p::test(port, program_directory);
	}

	gui GUI;
	gui_main->run(GUI);
}
