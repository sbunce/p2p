#ifndef H_GUI_PREFERENCES
#define H_GUI_PREFERENCES

//custom
#include "client.h"
#include "server.h"

//Gtk
#include <gtkmm.h>

class gui_preferences : public Gtk::Window
{  
public:
	gui_preferences(client * Client_in, server * Server_in);

private:
	class Gtk::Window * window;
	class Gtk::Entry * download_directory_entry;
	class Gtk::Entry * share_directory_entry;
	class Gtk::Entry * download_speed_entry;
	class Gtk::Entry * upload_speed_entry;
	class Gtk::Label * downloads_label;
	class Gtk::Label * share_label;
	class Gtk::Label * speed_label;
	class Gtk::Label * connection_limit_label;
	class Gtk::Label * upload_speed_label;
	class Gtk::Label * download_speed_label;
	class Gtk::Button * ok_button;
	class Gtk::Button * cancel_button;
	class Gtk::Button * apply_button;
	class Gtk::HScale * connection_limit_hscale;
	class Gtk::Fixed * fixed;

	//holds pointers to the client/server the GUI has while preferences are open
	client * Client;
	server * Server;

	/*
	apply_click    - apply button clicked
	apply_settings - read input preferences and set them
	cancel_click   - cancel button clicked
	ok_click       - ok button clicked
	*/
	void apply_click();
	void apply_settings();
	void cancel_click();
	void ok_click();
};
#endif
