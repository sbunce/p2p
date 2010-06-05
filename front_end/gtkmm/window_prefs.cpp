#include "window_prefs.hpp"

window_prefs::window_prefs(
	p2p & P2P_in
):
	P2P(P2P_in)
{
	max_download_rate_entry = Gtk::manage(new Gtk::Entry());
	max_upload_rate_entry = Gtk::manage(new Gtk::Entry());
	connections_hscale = Gtk::manage(new Gtk::HScale(2,1001,1));
	Gtk::Label * rate_label = Gtk::manage(new Gtk::Label("Rate Limit (kB/s)"));
	Gtk::Label * connection_limit_label = Gtk::manage(new Gtk::Label("Connection Limit"));
	Gtk::Label * upload_rate_label = Gtk::manage(new Gtk::Label("U:"));
	Gtk::Label * download_rate_label = Gtk::manage(new Gtk::Label("D:"));
	Gtk::Fixed * fixed = Gtk::manage(new Gtk::Fixed());

	//add/compose elements
	this->add(*fixed);
	fixed->put(*max_download_rate_entry, 40, 40);
	fixed->put(*max_upload_rate_entry, 40, 80);
	fixed->put(*rate_label, 4, 10);
	fixed->put(*connection_limit_label, 4, 120);
	fixed->put(*download_rate_label, 8, 45);
	fixed->put(*upload_rate_label, 8, 85);
	fixed->put(*connections_hscale, 8, 150);

	//set current options
	std::stringstream ss;
	ss << P2P.get_max_download_rate() / 1024;
	max_download_rate_entry->set_text(ss.str());
	ss.str(""); ss.clear();
	ss << P2P.get_max_upload_rate() / 1024;
	max_upload_rate_entry->set_text(ss.str());

	//options
	this->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	max_download_rate_entry->set_size_request(75,23);
	max_upload_rate_entry->set_size_request(75,23);
	rate_label->set_size_request(142,17);
	connection_limit_label->set_size_request(143,17);
	upload_rate_label->set_size_request(38,17);
	download_rate_label->set_size_request(38,17);
	connections_hscale->set_size_request(200,37);
	connections_hscale->set_draw_value(true);
	connections_hscale->set_value_pos(Gtk::POS_BOTTOM);
	connections_hscale->set_value(P2P.get_max_connections());

	//signaled functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &window_prefs::apply_settings), 1000);

	this->show_all_children();
}

bool window_prefs::apply_settings()
{
	std::stringstream ss;
	unsigned download_rate;
	ss << max_download_rate_entry->get_text();
	ss >> download_rate;

	ss.str(""); ss.clear();
	unsigned upload_rate;
	ss << max_upload_rate_entry->get_text();
	ss >> upload_rate;

	P2P.set_max_download_rate(download_rate * 1024);
	P2P.set_max_connections(connections_hscale->get_value());
	P2P.set_max_upload_rate(upload_rate * 1024);

	return true;
}
