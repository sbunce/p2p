#ifndef H_GUI_PREFERENCES
#define H_GUI_PREFERENCES

//Gtk
#include <gtkmm.h>

class gui_preferences : public Gtk::Window
{  
public:
	gui_preferences();

private:
	class Gtk::Window * window;
	class Gtk::Entry * downloads_entry;
	class Gtk::Entry * share_entry;
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

	/*
	close_window - close this window
	*/
	void close_window();
	void apply_click();
	void ok_click();
};
#endif
