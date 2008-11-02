#ifndef H_GUI_DOWNLOAD_STATUS
#define H_GUI_DOWNLOAD_STATUS

//boost
#include <boost/shared_ptr.hpp>

//custom
#include "client.h"
#include "convert.h"
#include "download_info.h"
#include "global.h"

//Gtk
#include <gtkmm.h>

//std
#include <string>

class gui_download_status : public Gtk::ScrolledWindow
{
public:
	gui_download_status(
		const std::string root_hash_in,
		Gtk::Label * tab_label,         //needs to be set to name of file
		client * Client_in
	);

	/*
	refresh - update all values, stops refreshing when refresh set to false
	*/
	bool refresh();

private:
	//pointer to client that exists in GUI class
	client * Client;

	std::string root_hash;
	boost::uint64_t tree_size_bytes;
	boost::uint64_t file_size_bytes;

	class Gtk::ScrolledWindow * window; //ptr to this
	Gtk::VBox * vbox;                   //divides top/bottom

	//top stuff
	Gtk::Fixed * info_fixed;
	Gtk::Label * root_hash_label;
	Gtk::Label * root_hash_value;
	Gtk::Label * hash_tree_size_label;
	Gtk::Label * hash_tree_size_value;
	Gtk::Label * hash_tree_percent_label;
	Gtk::Label * hash_tree_percent_value;
	Gtk::Label * file_name_label;
	Gtk::Label * file_name_value;
	Gtk::Label * file_size_label;
	Gtk::Label * file_size_value;
	Gtk::Label * file_percent_label;
	Gtk::Label * file_percent_value;
	Gtk::Label * total_speed_label;
	Gtk::Label * total_speed_value;
	Gtk::Label * servers_connected_label;
	Gtk::Label * servers_connected_value;

	//bottom server stuff
	Gtk::ScrolledWindow * servers_scrolled_window;
	Gtk::TreeView * servers_view;
	Glib::RefPtr<Gtk::ListStore> servers_list;
};
#endif
