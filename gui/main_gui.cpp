//custom
#include "gui.hpp"

//gtkmm
#include <gtkmm.h>

int main(int argc, char ** argv)
{
	Gtk::Main main(argc, argv);
	gui GUI;
	main.run(GUI);
}
