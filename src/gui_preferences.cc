//custom
#include "global.h"

#include "gui_preferences.h"

gui_preferences::gui_preferences()
{
	window = this;
	downloads_entry = Gtk::manage(new Gtk::Entry());
	share_entry = Gtk::manage(new Gtk::Entry());
	download_speed_entry = Gtk::manage(new Gtk::Entry());
	upload_speed_entry = Gtk::manage(new Gtk::Entry());
	downloads_label = Gtk::manage(new Gtk::Label("Download Directory"));
	share_label = Gtk::manage(new Gtk::Label("Share Directory"));
	speed_label = Gtk::manage(new Gtk::Label("Speed Limits (kB/s)"));
	connection_limit_label = Gtk::manage(new Gtk::Label("Connection Limit"));
	upload_speed_label = Gtk::manage(new Gtk::Label("UL"));
	download_speed_label = Gtk::manage(new Gtk::Label("DL"));
	apply_button = Gtk::manage(new Gtk::Button(Gtk::StockID("gtk-apply")));
	cancel_button = Gtk::manage(new Gtk::Button(Gtk::StockID("gtk-cancel")));
	ok_button = Gtk::manage(new Gtk::Button(Gtk::StockID("gtk-ok")));
	connection_limit_hscale = Gtk::manage(new Gtk::HScale(1,1001,1));
	fixed = Gtk::manage(new Gtk::Fixed());

	window->resize(400, 300);
	window->set_title("Preferences");
	window->set_modal(true);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	window->add(*fixed);

	downloads_entry->set_size_request(320,23);
//	downloads_entry->set_text("");

	share_entry->set_size_request(320,23);
//	share_entry->set_text("");

	download_speed_entry->set_size_request(75,23);
//	download_speed_entry->set_text("");

	upload_speed_entry->set_size_request(75,23);
//	upload_speed_entry->set_text("");

	downloads_label->set_size_request(141,17);
	downloads_label->set_alignment(0.5,0.5);
	downloads_label->set_justify(Gtk::JUSTIFY_LEFT);
	downloads_label->set_line_wrap(false);
	downloads_label->set_use_markup(false);
	downloads_label->set_selectable(false);

	share_label->set_size_request(116,17);
	share_label->set_alignment(0.5,0.5);
	share_label->set_justify(Gtk::JUSTIFY_LEFT);
	share_label->set_line_wrap(false);
	share_label->set_use_markup(false);
	share_label->set_selectable(false);
	speed_label->set_size_request(142,17);
	speed_label->set_alignment(0.5,0.5);
	speed_label->set_justify(Gtk::JUSTIFY_LEFT);
	speed_label->set_line_wrap(false);
	speed_label->set_use_markup(false);
	speed_label->set_selectable(false);

	connection_limit_label->set_size_request(143,17);
	connection_limit_label->set_alignment(0.5,0.5);
	connection_limit_label->set_justify(Gtk::JUSTIFY_LEFT);
	connection_limit_label->set_line_wrap(false);
	connection_limit_label->set_use_markup(false);
	connection_limit_label->set_selectable(false);

	upload_speed_label->set_size_request(38,17);
	upload_speed_label->set_alignment(0.5,0.5);
	upload_speed_label->set_justify(Gtk::JUSTIFY_LEFT);
	upload_speed_label->set_line_wrap(false);
	upload_speed_label->set_use_markup(false);
	upload_speed_label->set_selectable(false);

	download_speed_label->set_size_request(38,17);
	download_speed_label->set_alignment(0.5,0.5);
	download_speed_label->set_justify(Gtk::JUSTIFY_LEFT);
	download_speed_label->set_line_wrap(false);
	download_speed_label->set_use_markup(false);
	download_speed_label->set_selectable(false);

	connection_limit_hscale->set_size_request(200,37);
	connection_limit_hscale->set_draw_value(true);
	connection_limit_hscale->set_value_pos(Gtk::POS_BOTTOM);

	fixed->put(*downloads_entry, 16, 40);
	fixed->put(*share_entry, 16, 104);
	fixed->put(*download_speed_entry, 40, 184);
	fixed->put(*upload_speed_entry, 40, 216);
	fixed->put(*downloads_label, 8, 16);
	fixed->put(*share_label, 8, 80);
	fixed->put(*speed_label, 8, 152);
	fixed->put(*connection_limit_label, 160, 152);
	fixed->put(*upload_speed_label, 8, 216);
	fixed->put(*download_speed_label, 8, 184);
	fixed->put(*connection_limit_hscale, 176, 192);
	fixed->put(*ok_button, 332, 256);
	fixed->put(*cancel_button, 252, 256);
	fixed->put(*apply_button, 180, 256);

	show_all_children();

	//signaled functions
	apply_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::apply_click), false);
	cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::close_window), false);
	ok_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::ok_click), false);
}

void gui_preferences::apply_click()
{
	std::cout << "apply button clicked\n";
}

void gui_preferences::close_window()
{
	hide();
}

void gui_preferences::ok_click()
{
	std::cout << "ok button clicked\n";
}
