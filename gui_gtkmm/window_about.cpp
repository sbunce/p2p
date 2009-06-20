#include "window_about.hpp"

window_about::window_about()
{
	window = this;

	vbox = Gtk::manage(new Gtk::VBox(false, 0));
	scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	text_view = Gtk::manage(new Gtk::TextView);
	button_box = Gtk::manage(new Gtk::HButtonBox);
	close_button = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));

	window->set_icon(
		Gtk::Widget::render_icon(Gtk::Stock::ABOUT, Gtk::ICON_SIZE_LARGE_TOOLBAR)
	);

	window->set_title(("About P2P"));
	window->resize(500, 350);
	window->set_modal(true);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER);

	add(*vbox);
	scrolled_window->add(*text_view);
	scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*scrolled_window);

	text_buff = Gtk::TextBuffer::create();
	text_buff->set_text(settings::NAME+" version: "+settings::VERSION+" © Seth Bunce 2006");
	text_view->set_buffer(text_buff);
	text_view->set_editable(false);

	vbox->pack_start(*button_box, Gtk::PACK_SHRINK);
	button_box->pack_start(*close_button, Gtk::PACK_SHRINK);

	//signaled functions
	close_button->signal_clicked().connect(sigc::mem_fun(*this, &window_about::close_click));

	show_all_children();
}

void window_about::close_click()
{
	hide();
}
