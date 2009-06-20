#ifndef H_WINDOW_ABOUT
#define H_WINDOW_ABOUT

//custom
#include "settings.hpp"

//boost
#include <boost/utility.hpp>

//p2p
#include <p2p/p2p.hpp>

//gtkmm
#include <gtkmm.h>

class window_about : public Gtk::Window, private boost::noncopyable
{
public:
	window_about();

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
