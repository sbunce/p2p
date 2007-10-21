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
	if(argc > 1){
		if(!strcmp(argv[1],"-s")){
			server Server;
			Server.start();

			while(true){
				sleep(1);
			}
		}
	}

	Gtk::Main main(argc, argv);
	gui * GUI = new gui;
	main.run(*GUI);
	delete GUI;

	return 0;
}
