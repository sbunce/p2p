//Gtk
#include <gtkmm.h>

#include "gui.h"
#include "server.h"

int main(int argc, char* argv[])
{
	Gtk::Main main(argc, argv);
	gui * GUI = new gui;
	main.run(*GUI);
	delete GUI;

	return 0;
}
