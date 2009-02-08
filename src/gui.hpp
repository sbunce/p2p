#ifndef H_GUI
#define H_GUI

//boost
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//custom
#include "client.hpp"
#include "convert.hpp"
#include "database.hpp"
#include "download_info.hpp"
#include "global.hpp"
#include "gui_about.hpp"
#include "gui_vbox_search.hpp"
#include "gui_window_download_status.hpp"
#include "gui_window_preferences.hpp"
#include "server.hpp"
#include "upload_info.hpp"

//gui
#include <gtkmm.h>

//std
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

class gui : public Gtk::Window, private boost::noncopyable
{
public:
	gui();
	~gui();

private:
	Gtk::Window * window;
	Gtk::MenuBar * menubar;
	Gtk::Notebook * notebook;

	//file menu
	Gtk::Menu * file_menu;
	Gtk::MenuItem * file_menu_item;
	Gtk::ImageMenuItem * quit;

	//settings menu
	Gtk::Menu * settings_menu;
	Gtk::MenuItem * settings_menu_item;
	Gtk::MenuItem * preferences;

	//help menu
	Gtk::Menu * help_menu;
	Gtk::MenuItem * help_menu_item;
	Gtk::MenuItem * about;

	//notebook labels
	Gtk::Image * search_label;
	Gtk::Image * download_label;
	Gtk::Image * upload_label;
	Gtk::Image * tracker_label;

	//treeviews for different tabs
	Gtk::TreeView * download_view;
	Gtk::ScrolledWindow * download_scrolled_window;
	Gtk::TreeView * upload_view;
	Gtk::ScrolledWindow * upload_scrolled_window;

	//boxes (divides the window)
	Gtk::VBox * main_VBox;    //VBox for the main window (separates top from bottom)

	//bottom bar that displays status etc
	Gtk::Statusbar * statusbar;

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloads_popup_menu;

	//the lists which the treeviews display
	Glib::RefPtr<Gtk::ListStore> download_list;
	Glib::RefPtr<Gtk::ListStore> upload_list;

	/*
	Custom classes derived from gtkmm objects.
	*/
	gui_vbox_search * GUI_VBox_Search;

	/*functions associated with Gtk signals
	help_about              - user clicked Help/About
	cancel_download         - when user clicks Cancel on a download in download_view
	compare_file_size       - custom compare function for file sizes
	download_click          - interprets download_view input, can pop up a menu on right click
	download_file           - when user clicks Download on a entry in search_view
	download_info_tab       - opens a tab for detailed download information
	download_info_tab_close - closes a download info tab
	on_quit                 - when user clicks File/Quit
	on_delete_event         - overrides the event which happens on window close
	search_input            - when user clicks search_button or hits enter while on search_entry
	search_click            - interprets search_view input, can pop up a menu on right click
	settings_preferences    - user clicked Settings/Preferences
	*/
	void help_about();
	void cancel_download();
	int compare_file_size(const Gtk::TreeModel::iterator & lval, const Gtk::TreeModel::iterator & rval);
	bool download_click(GdkEventButton * event);
	void download_info_tab();
	void download_info_tab_close(gui_window_download_status * status_window, sigc::connection tab_conn);
	void on_quit();
	bool on_delete_event(GdkEventAny * event);
	void search_input();
	bool search_click(GdkEventButton * event);
	void settings_preferences();

	/*
	download_info_setup   - sets up the download tab treeview columns
	download_info_refresh - updates the information in download_list
	upload_info_setup     - sets up the upload tab treeview columns
	upload_info_refresh   - updates the information in upload_list
	search_info_setup     - sets up the search tab treeview columns
	search_info_refresh   - updates the information in search_list
	update_status_bar     - updates the information in the status bar
	*/
	void download_info_setup();
	bool download_info_refresh();
	void upload_info_setup();
	bool upload_info_refresh();
	void search_info_setup();
	void search_info_refresh();
	bool update_status_bar();

	client Client;
	server Server;

	//information from the last search
	std::vector<download_info> Search_Info;
};
#endif
