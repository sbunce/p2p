//Gtk
#include <gtkmm.h>

#include "gui.h"

int main(int argc, char* argv[])
{
	Gtk::Main main(argc, argv);
	gui * GUI = new class gui;
	main.run(*GUI);
	delete GUI;

	return 0;
}
