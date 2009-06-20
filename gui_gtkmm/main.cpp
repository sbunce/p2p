//boost
#include <boost/shared_ptr.hpp>

//custom
#include "gui.hpp"

//gtkmm
#include <gtkmm.h>

//std
#include <csignal>

namespace
{
	Gtk::Main * Main_ptr;

	void signal_handler(int sig)
	{
		signal(sig, signal_handler);
		Main_ptr->quit();
	}
}//end of unnamed namespace

int main(int argc, char ** argv)
{
	Gtk::Main Main(argc, argv);
	Main_ptr = &Main;

	//register signal handlers
	signal(SIGINT, signal_handler);

	gui GUI;
	Main.run(GUI);
}
