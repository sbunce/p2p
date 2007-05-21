//Gtk
#include <gtkmm.h>

//std
#include <iostream>
#include <string>

#include "gui.h"

int main(int argc, char* argv[])
{
	Gtk::Main main(argc, argv);
	gui * GUI = new class gui;
	main.run(*GUI);
	delete GUI;

	return 0;
}
