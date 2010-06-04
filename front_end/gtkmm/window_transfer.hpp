#ifndef H_WINDOW_TRANSFER
#define H_WINDOW_TRANSFER

//custom
#include "settings.hpp"

//include
#include <convert.hpp>
#include <gtkmm.h>
#include <logger.hpp>
#include <p2p.hpp>

//standard
#include <map>
#include <set>
#include <string>

class window_transfer : public Gtk::ScrolledWindow, private boost::noncopyable
{
public:
	enum type {
		download,
		upload
	};

	window_transfer(
		p2p & P2P_in,
		const type Type_in
	);

private:
	p2p & P2P;

	//upload or download (some things are disabled for upload)
	const type Type;

	//objects for display of downloads
	Gtk::TreeView * download_view;
	Gtk::ScrolledWindow * download_scrolled_window;
	Glib::RefPtr<Gtk::ListStore> download_list;

	//treeview columns
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_name;
	Gtk::TreeModelColumn<Glib::ustring> column_size;
	Gtk::TreeModelColumn<Glib::ustring> column_speed;
	Gtk::TreeModelColumn<int> column_percent_complete;
	Gtk::TreeModelColumn<Glib::ustring> column_hash;
	Gtk::TreeModelColumn<bool> column_update;
	Gtk::CellRendererProgress cell;

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloads_popup_menu;

	//hash associated with row (this is used as index for fast updates)
	std::map<std::string, Gtk::TreeModel::Row> Row_Idx;

	/*
	compare_size:
		Signaled when user clicks size or speed column to sort.
	delete_download:
		Removes selected download.
	click:
		Called when TreeView clicked.
	refresh:
		Refreshes TreeView.
	pause:
		Pauses the selected download.
	*/
	int compare_size(const Gtk::TreeModel::iterator & lval,
		const Gtk::TreeModel::iterator & rval);
	void delete_download();
	bool click(GdkEventButton * event);
	bool refresh();
};
#endif
