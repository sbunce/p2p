#ifndef H_WINDOW_PREFS
#define H_WINDOW_PREFS

//include
#include <boost/utility.hpp>
#include <gtkmm.h>
#include <logger.hpp>
#include <p2p.hpp>

//standard
#include <sstream>

class window_prefs : public Gtk::ScrolledWindow, private boost::noncopyable
{  
public:
	window_prefs(p2p & P2P_in);

private:
	p2p & P2P;

	Gtk::Entry * max_download_rate_entry;
	Gtk::Entry * max_upload_rate_entry;
	Gtk::HScale * connections_hscale;

	/*
	apply_settings:
		Called once per second to apply settings. This makes it so we don't need
		an apply button.
	*/
	bool apply_settings();
};
#endif
