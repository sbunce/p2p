#ifndef H_GUI_VBOX_SEARCH
#define H_GUI_VBOX_SEARCH

//boost
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//custom
#include "download_info.hpp"
#include "p2p.hpp"
#include "settings.hpp"

//gui
#include <gtkmm.h>

//include
#include <convert.hpp>

//std
#include <algorithm>
#include <string>

class gui_vbox_search : public Gtk::VBox, private boost::noncopyable
{
public:
	gui_vbox_search(p2p & P2P_in);

private:
	//convenience pointer to this
	Gtk::VBox * vbox;

	//objects for search entry
	Gtk::Entry * search_entry;
	Gtk::Button * search_button;
	Gtk::HBox * search_HBox;     //separates search input and search button

	//objects for display of search results
	Gtk::ScrolledWindow * search_scrolled_window; //scrolled window for search_view
	Gtk::TreeView * search_view;
	Glib::RefPtr<Gtk::ListStore> search_list;     //list to display with search_view
	Gtk::Menu search_popup_menu;                  //right click popup menu

	//columns of the treeview
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_hash;
	Gtk::TreeModelColumn<Glib::ustring> column_name;
	Gtk::TreeModelColumn<Glib::ustring> column_size;
	Gtk::TreeModelColumn<Glib::ustring> column_IP;

	//pointer to client that exists in gui class
	p2p * P2P;

	/*
	Signaled Functions:
	download_file - "download" on the popup menu clicked
	search_input  - search button clicked or user hit enter with serach_entry selected
	search_click  - right click on search_view
	*/
	void download_file();
	void search_input();
	bool search_click(GdkEventButton * event);

	/*
	compare_file_size - -1 less than, 0 equal, 1 greater than, just like strcmp
	*/
	int compare_file_size(const Gtk::TreeModel::iterator & lval, const Gtk::TreeModel::iterator & rval);
	void search_info_refresh();

	//information from the last search
	std::vector<download_info> Search_Info;
};
#endif
