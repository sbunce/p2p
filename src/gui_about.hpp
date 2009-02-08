#ifndef H_GUI_ABOUT
#define H_GUI_ABOUT

//boost
#include <boost/utility.hpp>

//custom
#include "global.hpp"

//gui
#include <gtkmm.h>

class gui_about : public Gtk::Window, private boost::noncopyable
{
public:
	gui_about();

private:
	Gtk::Window * window;
	Gtk::VBox * vbox;
	Gtk::ScrolledWindow * scrolled_window;
	Gtk::TextView * text_view;
	Glib::RefPtr<Gtk::TextBuffer> text_buff;
	Gtk::HButtonBox * button_box;
	Gtk::Button * close_button;

	/*
	close_click - close button clicked
	*/
	void close_click();
};
#endif
