#ifndef H_STATUSBAR_MAIN
#define H_STATUSBAR_MAIN

//custom
#include "settings.hpp"

//include
#include <boost/utility.hpp>
#include <convert.hpp>
#include <gtkmm.h>
#include <p2p.hpp>

//standard
#include <iomanip>
#include <sstream>
#include <string>

class statusbar_main : public Gtk::Statusbar, private boost::noncopyable
{
public:
	statusbar_main(p2p & P2P_in);

private:
	p2p & P2P;

	//convenience pointer
	Gtk::Statusbar * statusbar;

	/*
	update_status_bar:
		Updates the information in the status bar.
	*/
	bool update_status_bar();
};
#endif
