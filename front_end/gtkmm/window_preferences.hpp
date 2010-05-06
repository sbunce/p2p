#ifndef H_WINDOW_PREFERENCES
#define H_WINDOW_PREFERENCES

//custom
#include "settings.hpp"

//include
#include <boost/utility.hpp>
#include <gtkmm.h>
#include <logger.hpp>
#include <p2p.hpp>

//standard
#include <sstream>

class window_preferences : public Gtk::Window, private boost::noncopyable
{  
public:
	window_preferences(p2p & P2P_in);

private:
	p2p & P2P;

	Gtk::Window * window;
	Gtk::Entry * max_download_rate_entry;
	Gtk::Entry * max_upload_rate_entry;
	Gtk::Label * rate_label;
	Gtk::Label * connection_limit_label;
	Gtk::Label * upload_rate_label;
	Gtk::Label * download_rate_label;
	Gtk::Button * ok_button;
	Gtk::Button * cancel_button;
	Gtk::Button * apply_button;
	Gtk::HButtonBox * button_box;
	Gtk::HScale * connections_hscale;
	Gtk::Fixed * fixed;

	/*
	apply_click:
		Apply button clicked.
	apply_settings:
		Read input preferences and set them.
	cancel_click:
		Cancel button clicked.
	client_connections_changed:
		Called when client_connections_hscale changes value.
	server_connections_changed:
		Called when server_connections_hscale changes value.
	ok_click:
		Ok button clicked.
	*/
	void apply_click();
	void apply_settings();
	void cancel_click();
	void client_connections_changed();
	void server_connections_changed();
	void ok_click();
};
#endif
