#ifndef H_STATUSBAR_MAIN
#define H_STATUSBAR_MAIN

//boost
#include <boost/utility.hpp>

//custom
#include "settings.hpp"

//gui
#include <gtkmm.h>

//include
#include <convert.hpp>

//p2p
#include <p2p/p2p.hpp>

//std
#include <sstream>
#include <string>

class statusbar_main : public Gtk::Statusbar, private boost::noncopyable
{
public:
	statusbar_main(p2p & P2P_in);

private:
	//convenience pointer
	Gtk::Statusbar * statusbar;

	//same client and server that exist in gui
	p2p & P2P;

	/*
	update_status_bar:
		Updates the information in the status bar.
	*/
	bool update_status_bar();
};
#endif
