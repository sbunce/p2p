#ifndef H_GUI_ABOUT
#define H_GUI_ABOUT

#include <gtkmm.h>

class gui_about : public Gtk::Window
{
public:
	gui_about();

private:
	class Gtk::Window * window;

	Gtk::VBox about_VBox;
	Gtk::ScrolledWindow about_scrolledWindow;
	Gtk::TextView about_textView;
	Glib::RefPtr<Gtk::TextBuffer> about_refTextBuff;

	Gtk::HButtonBox about_buttonBox;
	Gtk::Button buttonClose;

	void on_button_close();
};
#endif
