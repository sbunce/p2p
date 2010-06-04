#include "window_prefs.hpp"

window_prefs::window_prefs(
	p2p & P2P_in
):
	P2P(P2P_in)
{
	max_download_rate_entry = Gtk::manage(new Gtk::Entry());
	max_upload_rate_entry = Gtk::manage(new Gtk::Entry());
	connections_hscale = Gtk::manage(new Gtk::HScale(2,1001,1));

	Gtk::Label * rate_label = Gtk::manage(new Gtk::Label("Speed (kB/s)"));
	Gtk::Label * connection_limit_label = Gtk::manage(new Gtk::Label("Connection Limit"));
	Gtk::Label * upload_rate_label = Gtk::manage(new Gtk::Label("UL"));
	Gtk::Label * download_rate_label = Gtk::manage(new Gtk::Label("DL"));
	Gtk::Fixed * fixed = Gtk::manage(new Gtk::Fixed());

	this->add(*fixed);
	this->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC); //auto scroll bars

	std::stringstream ss;
	unsigned download_rate, upload_rate;
	ss << P2P.get_max_download_rate();
	ss >> download_rate;
	download_rate /= 1024;
	ss.str(""); ss.clear();
	ss << download_rate;
	max_download_rate_entry->set_size_request(75,23);
	max_download_rate_entry->set_text(ss.str());

	ss.str(""); ss.clear();
	ss << P2P.get_max_upload_rate();
	ss >> upload_rate;
	upload_rate /= 1024;
	ss.str(""); ss.clear();
	ss << upload_rate;
	max_upload_rate_entry->set_size_request(75,23);
	max_upload_rate_entry->set_text(ss.str());

	rate_label->set_size_request(142,17);
	rate_label->set_alignment(0.5,0.5);
	rate_label->set_justify(Gtk::JUSTIFY_LEFT);
	rate_label->set_line_wrap(false);
	rate_label->set_use_markup(false);
	rate_label->set_selectable(false);

	connection_limit_label->set_size_request(143,17);
	connection_limit_label->set_alignment(0.5,0.5);
	connection_limit_label->set_justify(Gtk::JUSTIFY_LEFT);
	connection_limit_label->set_line_wrap(false);
	connection_limit_label->set_use_markup(false);
	connection_limit_label->set_selectable(false);

	upload_rate_label->set_size_request(38,17);
	upload_rate_label->set_alignment(0.5,0.5);
	upload_rate_label->set_justify(Gtk::JUSTIFY_LEFT);
	upload_rate_label->set_line_wrap(false);
	upload_rate_label->set_use_markup(false);
	upload_rate_label->set_selectable(false);

	download_rate_label->set_size_request(38,17);
	download_rate_label->set_alignment(0.5,0.5);
	download_rate_label->set_justify(Gtk::JUSTIFY_LEFT);
	download_rate_label->set_line_wrap(false);
	download_rate_label->set_use_markup(false);
	download_rate_label->set_selectable(false);

	connections_hscale->set_size_request(200,37);
	connections_hscale->set_draw_value(true);
	connections_hscale->set_value_pos(Gtk::POS_TOP);
	connections_hscale->set_value(P2P.get_max_connections());

	fixed->put(*max_download_rate_entry, 40, 40);
	fixed->put(*max_upload_rate_entry, 40, 80);
	fixed->put(*rate_label, 4, 10);
	fixed->put(*connection_limit_label, 140, 10);
	fixed->put(*download_rate_label, 8, 45);
	fixed->put(*upload_rate_label, 8, 85);
	fixed->put(*connections_hscale, 140, 50);

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &window_prefs::apply_settings), 1000);

	show_all_children();
}

bool window_prefs::apply_settings()
{
	int download_rate, upload_rate;
	std::stringstream ss;
	ss << max_download_rate_entry->get_text();
	ss >> download_rate;
	ss.str(""); ss.clear();
	ss << max_upload_rate_entry->get_text();
	ss >> upload_rate;
	ss.str(""); ss.clear();

	P2P.set_max_download_rate(download_rate * 1024);
	P2P.set_max_connections((int)connections_hscale->get_value());
	P2P.set_max_upload_rate(upload_rate * 1024);

	return true;
}
