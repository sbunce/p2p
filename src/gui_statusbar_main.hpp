#ifndef H_GUI_STATUSBAR_MAIN
#define H_GUI_STATUSBAR_MAIN

//custom
#include "client.hpp"
#include "global.hpp"
#include "server.hpp"

//gui
#include <gtkmm.h>

//std
#include <sstream>
#include <string>

class gui_statusbar_main : public Gtk::Statusbar, private boost::noncopyable
{
public:
	gui_statusbar_main(
		client & Client_in,
		server & Server_in
	);

private:
	//convenience pointer
	Gtk::Statusbar * statusbar;

	//same client and server that exist in gui
	client * Client;
	server * Server;

	/*
	update_status_bar     - updates the information in the status bar
	*/
	bool update_status_bar();
};
#endif
