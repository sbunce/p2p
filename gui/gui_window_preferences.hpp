#ifndef H_GUI_WINDOW_PREFERENCES
#define H_GUI_WINDOW_PREFERENCES

//boost
#include <boost/utility.hpp>

//p2p
#include <p2p.hpp>

//gui
#include <gtkmm.h>

//std
#include <sstream>

class gui_window_preferences : public Gtk::Window, private boost::noncopyable
{  
public:
	gui_window_preferences(p2p & P2P_in);

private:
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

	//holds pointers to the client/server the GUI has while preferences are open
	p2p * P2P;

	/*
	apply_click                - apply button clicked
	apply_settings             - read input preferences and set them
	cancel_click               - cancel button clicked
	client_connections_changed - called when client_connections_hscale changes value
	server_connections_changed - called when server_connections_hscale changes value
	ok_click                   - ok button clicked
	*/
	void apply_click();
	void apply_settings();
	void cancel_click();
	void client_connections_changed();
	void server_connections_changed();
	void ok_click();
};
#endif
