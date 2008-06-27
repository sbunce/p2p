#include "gui_preferences.h"

gui_preferences::gui_preferences(client * Client_in, server * Server_in)
{
	Client = Client_in;
	Server = Server_in;

	window = this;
	download_directory_entry = Gtk::manage(new Gtk::Entry());
	share_directory_entry = Gtk::manage(new Gtk::Entry());
	download_speed_entry = Gtk::manage(new Gtk::Entry());
	upload_speed_entry = Gtk::manage(new Gtk::Entry());
	downloads_label = Gtk::manage(new Gtk::Label("Download Directory"));
	share_label = Gtk::manage(new Gtk::Label("Share Directory"));
	speed_label = Gtk::manage(new Gtk::Label("Speed (kB/s)"));
	connection_limit_label = Gtk::manage(new Gtk::Label("Connection Limit"));
	upload_speed_label = Gtk::manage(new Gtk::Label("UL"));
	download_speed_label = Gtk::manage(new Gtk::Label("DL"));
	apply_button = Gtk::manage(new Gtk::Button(Gtk::StockID("gtk-apply")));
	cancel_button = Gtk::manage(new Gtk::Button(Gtk::StockID("gtk-cancel")));
	ok_button = Gtk::manage(new Gtk::Button(Gtk::StockID("gtk-ok")));
	button_box = Gtk::manage(new Gtk::HButtonBox);
	connection_limit_hscale = Gtk::manage(new Gtk::HScale(1,1001,1));
	fixed = Gtk::manage(new Gtk::Fixed());

	window->set_resizable(false);
	window->set_title("Preferences");
	window->set_modal(true);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER);
	window->add(*fixed);

	download_directory_entry->set_size_request(320,23);
	download_directory_entry->set_text(Client->get_download_directory());

	share_directory_entry->set_size_request(320,23);
	share_directory_entry->set_text(Server->get_share_directory());

	download_speed_entry->set_size_request(75,23);
	download_speed_entry->set_text(Client->get_speed_limit());

	upload_speed_entry->set_size_request(75,23);
	upload_speed_entry->set_text(Server->get_speed_limit());

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
	assert(Server->get_max_connections() == Client->get_max_connections());
	connection_limit_hscale->set_value(Server->get_max_connections());

	button_box->set_spacing(8);
	button_box->set_border_width(8);
	button_box->pack_start(*apply_button);
	button_box->pack_start(*cancel_button);
	button_box->pack_start(*ok_button);

	fixed->put(*download_directory_entry, 16, 40);
	fixed->put(*share_directory_entry, 16, 104);
	fixed->put(*download_speed_entry, 40, 184);
	fixed->put(*upload_speed_entry, 40, 216);
	fixed->put(*downloads_label, 8, 16);
	fixed->put(*share_label, 8, 80);
	fixed->put(*speed_label, 4, 152);
	fixed->put(*connection_limit_label, 140, 152);
	fixed->put(*upload_speed_label, 8, 216);
	fixed->put(*download_speed_label, 8, 184);
	fixed->put(*connection_limit_hscale, 140, 192);
	fixed->put(*button_box, 80, 256);

	show_all_children();

	//signaled functions
	apply_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::apply_click), false);
	cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::cancel_click), false);
	ok_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::ok_click), false);
}

void gui_preferences::apply_click()
{
	apply_settings();
}

void gui_preferences::apply_settings()
{
	Client->set_download_directory(download_directory_entry->get_text());
	Client->set_speed_limit(download_speed_entry->get_text());
	Client->set_max_connections((int)connection_limit_hscale->get_value());
	Server->set_share_directory(share_directory_entry->get_text());
	Server->set_speed_limit(upload_speed_entry->get_text());
	Server->set_max_connections((int)connection_limit_hscale->get_value());
}

void gui_preferences::cancel_click()
{
	hide();
}

void gui_preferences::ok_click()
{
	apply_settings();
	hide();
}
