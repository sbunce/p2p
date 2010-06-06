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

	//bottom server stuff
	Gtk::ScrolledWindow * host_scrolled_window;
	Gtk::TreeView * host_view;
	Glib::RefPtr<Gtk::ListStore> host_list;

	//columns for treeview
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> IP_column;
	Gtk::TreeModelColumn<Glib::ustring> port_column;
	Gtk::TreeModelColumn<Glib::ustring> download_speed_column;
	Gtk::TreeModelColumn<Glib::ustring> upload_speed_column;

	/*
	compare_SI:
		Compares size SI for column sorting.
	refresh:
		Updates all information.
	*/
	int compare_SI(const Gtk::TreeModel::iterator & lval,
		const Gtk::TreeModel::iterator & rval, const Gtk::TreeModelColumn<Glib::ustring> column);
	bool refresh();
};
#endif
