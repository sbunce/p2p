#ifndef H_GUI_ABOUT
#define H_GUI_ABOUT

//Gtk
#include <gtkmm.h>

class gui_about : public Gtk::Window
{
public:
	gui_about();

	/*
	close_window - close this window
	*/
	void close_window();

private:
	class Gtk::Window * window;

	Gtk::VBox about_VBox;
	Gtk::ScrolledWindow about_scrolledWindow;
	Gtk::TextView about_textView;
	Glib::RefPtr<Gtk::TextBuffer> about_refTextBuff;

	Gtk::HButtonBox about_buttonBox;
	Gtk::Button buttonClose;
};
#endif
