//Gtk
#include <gtkmm.h>

//custom
#include "gui.h"

int main(int argc, char* argv[])
{
	Gtk::Main main(argc, argv);
	gui * GUI = new gui;
	main.run(*GUI);
	delete GUI;

	return 0;
}
