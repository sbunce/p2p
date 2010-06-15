//custom
#include "window_main.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <gtkmm.h>
#include <opt_parse.hpp>
#include <p2p.hpp>

//std
#include <csignal>

boost::shared_ptr<Gtk::Main> Main;
boost::once_flag once_flag = BOOST_ONCE_INIT;

void quit_prog()
{
	Main->quit();
}

void signal_handler(int sig)
{
	signal(sig, signal_handler);
	boost::call_once(once_flag, &quit_prog);
}

int main(int argc, char ** argv)
{
	opt_parse Opt_Parse(argc, argv);
	Main.reset(new Gtk::Main(argc, argv));

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

	//get last argument if *.p2p file
	boost::optional<std::string> file_path = Opt_Parse.match_last(".+\\.p2p");
	if(Opt_Parse.unparsed()){
		return 1;
	}
	if(file_path){
		p2p::load_file(*file_path);
		/*
		There is no good portable way of determining if the program is already
		running so we do not try to start the program after loading a file.
		*/
		return 0;
	}

	p2p P2P;
	window_main Window_Main(P2P);
	Main->run(Window_Main);
}
