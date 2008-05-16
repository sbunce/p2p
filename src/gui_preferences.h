#ifndef H_GUI_PREFERENCES
#define H_GUI_PREFERENCES

//Gtk
#include <gtkmm.h>

class gui_preferences : public Gtk::Window
{
public:
	gui_preferences();

	Gtk::VBox preferences_VBox;
	Gtk::HButtonBox buttonBox;
	Gtk::Button buttonClose;
	Gtk::Button buttonApply;

private:
	class Gtk::Window * window;

	void button_close();
	void button_apply();
};
#endif
