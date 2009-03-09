#ifndef H_GUI_WINDOW_DOWNLOAD_STATUS
#define H_GUI_WINDOW_DOWNLOAD_STATUS

//boost
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//custom
#include "convert.hpp"
#include "download_info.hpp"
#include "p2p.hpp"
#include "settings.hpp"

//gui
#include <gtkmm.h>

//std
#include <string>

class gui_window_download_status : public Gtk::ScrolledWindow, private boost::noncopyable
{
public:
	gui_window_download_status(
		const std::string & root_hash_in,
		const std::string & path,
		const boost::uint64_t & tree_size_in,
		const boost::uint64_t & file_size_in,
		Gtk::HBox *& hbox,           //set in ctor
		Gtk::Label *& tab_label,     //set in ctor
		Gtk::Button *& close_button, //set in ctor
		p2p & P2P_in
	);

	bool refresh();

private:
	//pointer to client that exists in GUI class
	p2p * P2P;

	std::string root_hash;
	boost::uint64_t tree_size;

	Gtk::ScrolledWindow * window;
	Gtk::VBox * vbox;
	Gtk::Image * close_image;

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

	//columns for treeview
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_server;
	Gtk::TreeModelColumn<Glib::ustring> column_speed;

	//bottom server stuff
	Gtk::ScrolledWindow * servers_scrolled_window;
	Gtk::TreeView * servers_view;
	Glib::RefPtr<Gtk::ListStore> servers_list;
};
#endif
