#ifndef H_GUI_STATUSBAR_MAIN
#define H_GUI_STATUSBAR_MAIN

//custom
#include "p2p.hpp"
#include "settings.hpp"

//gui
#include <gtkmm.h>

//std
#include <sstream>
#include <string>

class gui_statusbar_main : public Gtk::Statusbar, private boost::noncopyable
{
public:
	gui_statusbar_main(p2p & P2P_in);

private:
	//convenience pointer
	Gtk::Statusbar * statusbar;

	//same client and server that exist in gui
	p2p * P2P;

	/*
	update_status_bar     - updates the information in the status bar
	*/
	bool update_status_bar();
};
#endif
