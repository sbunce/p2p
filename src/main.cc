//Gtk
#include <gtkmm.h>

//std
#include <ctime>
#include <cstdlib>
#include <iostream>

#include "gui.h"
#include "server.h"

int main(int argc, char* argv[])
{
	if(argc == 1){
		Gtk::Main main(argc, argv);
		gui * GUI = new class gui;
		main.run(*GUI);
		delete GUI;
	}
	else{

		std::string arg1(argv[1]);
		if(arg1 == "-s"){
			server Server;
			Server.start();

			//run forever
			while(true){
				sleep(1);
			}
		}
	}

	return 0;
}
