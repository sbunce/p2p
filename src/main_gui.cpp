//custom
#include "CLI_args.hpp"
#include "global.hpp"
#include "server.hpp"
#include "gui.hpp"

//gtkmm
#include <gtkmm.h>

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);
	std::string help_message(
		"Usage: "+CLI_Args.program_name()+" <option>\n"+
		" --help        Pring usage information.\n"+
		" -s            No GUI, server only.\n"
	);
	CLI_Args.help_message("--help", help_message);

	if(CLI_Args.check_bool("-s")){
		//server only
		server Server;
		while(true){
			portable_sleep::ms(1000);
		}
	}else{
		//full program
		Gtk::Main main(argc, argv);
		gui * GUI = new gui;
		main.run(*GUI);
		delete GUI;
	}

	return 0;
}
