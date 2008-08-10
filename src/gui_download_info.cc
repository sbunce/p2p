#include "gui_download_info.h"

gui_download_info::gui_download_info()
{
	window = this;
	close_button = Gtk::manage(new Gtk::Button("Close"));
	button_box = Gtk::manage(new Gtk::HButtonBox);
	fixed = Gtk::manage(new Gtk::Fixed());

	window->set_title("Info");
	window->resize(500, 350);
	window->set_modal(true);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER);

	//signaled functions
	close_button->set_label("Close");
	close_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_download_info::close_click));

	show_all_children();
}

void gui_download_info::close_click()
{
	hide();
}
