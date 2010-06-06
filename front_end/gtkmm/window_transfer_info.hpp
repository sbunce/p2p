#ifndef H_WINDOW_TRANSFER_INFO
#define H_WINDOW_TRANSFER_INFO

//custom
#include "convert.hpp"
#include "settings.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <gtkmm.h>
#include <p2p.hpp>

//standard
#include <string>

class window_transfer_info : public Gtk::ScrolledWindow, private boost::noncopyable
{
public:
	window_transfer_info(
		p2p & P2P_in,
		const p2p::transfer_info & TI
	);

private:
	p2p & P2P;
	const std::string hash;

	Gtk::Label * hash_value;
	Gtk::Label * tree_size_value;
	Gtk::Label * tree_percent_value;
	Gtk::Label * file_name_value;
	Gtk::Label * file_size_value;
	Gtk::Label * file_percent_value;
	Gtk::Label * download_speed_value;
	Gtk::Label * upload_speed_value;

	//columns for treeview
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_server;
	Gtk::TreeModelColumn<Glib::ustring> column_speed;

	//bottom server stuff
	Gtk::ScrolledWindow * servers_scrolled_window;
	Gtk::TreeView * servers_view;
	Glib::RefPtr<Gtk::ListStore> servers_list;

	/*
	refresh:
		Updates all information.
	*/
	bool refresh();
};
#endif
