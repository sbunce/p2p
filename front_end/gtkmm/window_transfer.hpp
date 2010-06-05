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
	enum type_t {
		download,
		upload
	};

	window_transfer(
		const type_t type_in,
		p2p & P2P_in,
		Gtk::Notebook * notebook_in
	);

private:
	const type_t type;
	p2p & P2P;

	//needed to control adding and removing tabs
	Gtk::Notebook * notebook;

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

	//hash associated with row
	std::map<std::string, Gtk::TreeModel::Row> Row_Idx;

	/*
	compare_size:
		Signaled when user clicks size or speed column to sort.
	click:
		Called when TreeView clicked.
	download_delete:
		Called when delete selected from right click menu.
		Note: Only for downloads.
	transfer_info:
		Called when info selected from right click menu.
	refresh:
		Refreshes TreeView.
	pause:
		Pauses the selected download.
	*/
	int compare_size(const Gtk::TreeModel::iterator & lval,
		const Gtk::TreeModel::iterator & rval);
	bool click(GdkEventButton * event);
	void download_delete();
	void transfer_info();
	bool refresh();
};
#endif
