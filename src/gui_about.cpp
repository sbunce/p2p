#include "gui_about.hpp"

gui_about::gui_about()
{
	window = this;

	vbox = Gtk::manage(new Gtk::VBox(false, 0));
	scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	text_view = Gtk::manage(new Gtk::TextView);
	button_box = Gtk::manage(new Gtk::HButtonBox);
	close_button = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));

	window->set_title(("About " + global::NAME));
	window->resize(500, 350);
	window->set_modal(true);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER);

	add(*vbox);
	scrolled_window->add(*text_view);
	scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*scrolled_window);

	text_buff = Gtk::TextBuffer::create();
	text_buff->set_text(global::NAME + " version: " + global::VERSION + " © Seth Bunce 2006");
	text_view->set_buffer(text_buff);

	vbox->pack_start(*button_box, Gtk::PACK_SHRINK);
	button_box->pack_start(*close_button, Gtk::PACK_SHRINK);

	//signaled functions
	close_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_about::close_click));

	show_all_children();
}

void gui_about::close_click()
{
	hide();
}
