#ifndef H_GUI_VBOX_SEARCH
#define H_GUI_VBOX_SEARCH

//boost
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//custom
#include "client.hpp"
#include "convert.hpp"
#include "download_info.hpp"
#include "global.hpp"

//gui
#include <gtkmm.h>

//std
#include <string>

class gui_vbox_search : public Gtk::VBox, private boost::noncopyable
{
public:
	gui_vbox_search(client * Client_in);

private:
	//pointer to client that exists in gui class
	client * Client;

	Gtk::ScrolledWindow * search_scrolled_window; //scrolled window for treeview
	Gtk::TreeView * search_view;
	Glib::RefPtr<Gtk::ListStore> search_list;

	Gtk::Entry * search_entry;
	Gtk::Button * search_button;
	Gtk::HBox * search_HBox;     //separates search input and search button
	Gtk::Menu search_popup_menu; //right click popup menu

	/*
	Signaled Functions:

	*/
	void download_file();
	void search_input();
	bool search_click(GdkEventButton * event);

	/*
	
	*/
	int compare_file_size(const Gtk::TreeModel::iterator & lval, const Gtk::TreeModel::iterator & rval);
	void search_info_setup();
	void search_info_refresh();

	//information from the last search
	std::vector<download_info> Search_Info;
};
#endif
