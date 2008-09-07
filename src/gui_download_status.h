#ifndef H_GUI_DOWNLOAD_STATUS
#define H_GUI_DOWNLOAD_STATUS

//custom
#include "global.h"

//Gtk
#include <gtkmm.h>

class gui_download_status : public Gtk::ScrolledWindow
{
public:
	gui_download_status();

	/*
	refresh - update all values in the ScrolledWindow
	*/
	void refresh();

private:
	class Gtk::ScrolledWindow * window;  //ptr to this

	//main vbox divides download info (top) and server info (bottom)
	Gtk::VBox * vbox;

	//top stuff
	Gtk::Fixed * info_fixed;
	Gtk::Label * root_hash;
	Gtk::Label * root_hash_value;
	Gtk::Label * hash_tree_size;
	Gtk::Label * hash_tree_size_value;
	Gtk::Label * hash_tree_percent;
	Gtk::Label * hash_tree_percent_value;
	Gtk::Label * file_name;
	Gtk::Label * file_name_value;
	Gtk::Label * file_size;
	Gtk::Label * file_size_value;
	Gtk::Label * file_percent;
	Gtk::Label * file_percent_value;
	Gtk::Label * total_speed;
	Gtk::Label * total_speed_value;
	Gtk::Label * servers_connected;
	Gtk::Label * servers_connected_value;

	//bottom server stuff
	Gtk::ScrolledWindow * servers_scrolled_window;
	Gtk::TreeView * servers_tree_view;
};
#endif
