#include <gtkmm.h>

#include "gui_about.h"

gui_about::gui_about()
{
	window = this;

	window->set_title(("About simple p2p"));
	window->resize(300, 200);
	window->set_modal(false);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER_ALWAYS);
}
