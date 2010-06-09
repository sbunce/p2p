#ifndef H_WINDOW_TRANSFER
#define H_WINDOW_TRANSFER

//custom
#include "settings.hpp"
#include "window_transfer_info.hpp"

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
	Gtk::TreeView * transfer_view;
	Gtk::ScrolledWindow * transfer_scrolled_window;
	Glib::RefPtr<Gtk::ListStore> transfer_list;

	//treeview columns
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> name_column;
	Gtk::TreeModelColumn<Glib::ustring> size_column;
	Gtk::TreeModelColumn<Glib::ustring> speed_column;
	Gtk::TreeModelColumn<int> percent_complete_column;
	Gtk::TreeModelColumn<Glib::ustring> hash_column;
	Gtk::TreeModelColumn<bool> update_column;
	Gtk::CellRendererProgress cell;

	//popup menus for when user right clicks on treeviews
	Gtk::Menu popup_menu;

	//hash associated with row
	std::map<std::string, Gtk::TreeModel::Row> Row_Idx;

	/*
	compare_SI:
		Compares size SI for column sorting.
	click:
		Called when TreeView clicked.
	download_delete:
		Called when delete selected from right click menu.
		Note: Only for downloads.
	info_tab_close:
		Called when download info tab is closed.
	refresh:
		Refreshes TreeView.
	transfer_info:
		Called when info selected from right click menu.
	*/
	int compare_SI(const Gtk::TreeModel::iterator & lval,
		const Gtk::TreeModel::iterator & rval, const Gtk::TreeModelColumn<Glib::ustring> col);
	bool click(GdkEventButton * event);
	void download_delete();
	void info_tab_close(Gtk::ScrolledWindow * info_window);
	void transfer_info();
	bool refresh();
};
#endif
