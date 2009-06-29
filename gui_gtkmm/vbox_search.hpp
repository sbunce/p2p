#ifndef H_VBOX_SEARCH
#define H_VBOX_SEARCH

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <convert.hpp>
#include <gtkmm.h>
#include <p2p/p2p.hpp>

//standard
#include <algorithm>
#include <string>

class vbox_search : public Gtk::VBox, private boost::noncopyable
{
public:
	vbox_search(p2p & P2P_in);

private:
	p2p & P2P;

	//convenience pointer to this
	Gtk::VBox * vbox;

	//objects for search entry
	Gtk::Entry * search_entry;
	Gtk::Button * search_button;
	Gtk::HBox * search_HBox;     //separates search input and search button

	//objects for display of search results
	Gtk::ScrolledWindow * search_scrolled_window; //scrolled window for search_view
	Gtk::TreeView * search_view;
	Glib::RefPtr<Gtk::ListStore> search_list;     //list displayed with search_view
	Gtk::Menu search_popup_menu;

	//columns of the treeview
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_hash;
	Gtk::TreeModelColumn<Glib::ustring> column_name;
	Gtk::TreeModelColumn<Glib::ustring> column_size;
	Gtk::TreeModelColumn<Glib::ustring> column_IP;

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
