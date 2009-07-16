#ifndef H_WINDOW_UPLOAD
#define H_WINDOW_UPLOAD

//custom
#include "settings.hpp"

//include
#include <convert.hpp>
#include <gtkmm.h>
#include <p2p/p2p.hpp>

class window_upload : public Gtk::ScrolledWindow, private boost::noncopyable
{
public:
	window_upload(p2p & P2P_in);

private:
	p2p & P2P;

	//convenience pointer
	Gtk::ScrolledWindow * window;

	//objects for display of uploads
	Gtk::TreeView * upload_view;
	Glib::RefPtr<Gtk::ListStore> upload_list;

	//columns
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_hash;
	Gtk::TreeModelColumn<Glib::ustring> column_IP;
	Gtk::TreeModelColumn<Glib::ustring> column_name;
	Gtk::TreeModelColumn<Glib::ustring> column_size;
	Gtk::TreeModelColumn<Glib::ustring> column_speed;
	Gtk::TreeModelColumn<int> column_percent_complete;
	Gtk::CellRendererProgress * cell;

	/*
	upload_info_refresh:
		Refreshes upload_view.
	*/
	bool upload_info_refresh();
};
#endif
