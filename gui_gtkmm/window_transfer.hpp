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
		Gtk::Notebook * notebook_in,
		const type Type_in
	);

private:
	p2p & P2P;

	//the same notebook that exists in GUI
	Gtk::Notebook * notebook;

	//upload or download (some things are disabled for upload)
	const type Type;

	//convenience pointer
	Gtk::ScrolledWindow * window;

	//objects for display of downloads
	Gtk::TreeView * download_view;
	Gtk::ScrolledWindow * download_scrolled_window;
	Glib::RefPtr<Gtk::ListStore> download_list;

	//treeview columns
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_hash;
	Gtk::TreeModelColumn<Glib::ustring> column_name;
	Gtk::TreeModelColumn<Glib::ustring> column_size;
	Gtk::TreeModelColumn<Glib::ustring> column_peers;
	Gtk::TreeModelColumn<Glib::ustring> column_speed;
	Gtk::TreeModelColumn<int> column_percent_complete;
	Gtk::CellRendererProgress * cell; //percent renderer

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloads_popup_menu;

	//hashes of all opened tabs
	std::set<std::string> open_info_tabs;

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
	void pause();
};
#endif
