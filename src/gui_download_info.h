#ifndef H_GUI_DOWNLOAD_INFO
#define H_GUI_DOWNLOAD_INFO

//custom
#include "global.h"

//Gtk
#include <gtkmm.h>

class gui_download_info : public Gtk::Window
{
public:
	gui_download_info();

private:
	class Gtk::Window * window;
	Gtk::Button * close_button;
	Gtk::HButtonBox * button_box;
	Gtk::Fixed * fixed;

	/*
	close_click - close button clicked
	*/
	void close_click();
};
#endif
