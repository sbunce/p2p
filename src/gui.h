#ifndef H_GUI
#define H_GUI

//custom
#include "client.h"
#include "download_info.h"
#include "global.h"
#include "gui_about.h"
#include "gui_download_status.h"
#include "gui_preferences.h"
#include "server.h"
#include "upload_info.h"

//Gtk
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

class gui : public Gtk::Window
{
public:
	gui();
	~gui();

private:
	Gtk::Window * window;
	Gtk::MenuBar * menubar;
	Gtk::Notebook * notebook; //main tabs, holds Gtk::Label

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
	
	//search
	Gtk::Entry * search_entry;   //input box for searches
	Gtk::Button * search_button; //search button

	//tracker add
	Gtk::Entry * tracker_entry;   //input box for trackers
	Gtk::Button * tracker_button; //tracker button

	//treeviews for different tabs
	Gtk::TreeView * search_view;
	Gtk::ScrolledWindow * search_scrolled_window;
	Gtk::TreeView * download_view;
	Gtk::ScrolledWindow * download_scrolled_window;
	Gtk::TreeView * upload_view;
	Gtk::ScrolledWindow * upload_scrolled_window;
	Gtk::TreeView * tracker_view;
	Gtk::ScrolledWindow * tracker_scrolled_window;

	//boxes (divides the window)
	Gtk::VBox * main_VBox;    //VBox for the main window (separates top from bottom)
	Gtk::HBox * search_HBox;  //separates the search input and search button
	Gtk::HBox * tracker_HBox; //separates the tracker input and add button
	Gtk::VBox * search_VBox;  //VBox which goes inside search_scrolled_window
	Gtk::VBox * tracker_VBox; //VBox which goes inside the tracker_scrolled_window

	//bottom bar that displays status etc
	Gtk::Statusbar * statusbar;

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloads_popup_menu;
	Gtk::Menu search_popup_menu;

	//the lists which the treeviews display
	Glib::RefPtr<Gtk::ListStore> search_list;
	Glib::RefPtr<Gtk::ListStore> download_list;
	Glib::RefPtr<Gtk::ListStore> upload_list;
	Glib::RefPtr<Gtk::ListStore> tracker_list;

	/*functions associated with Gtk signals
	help_about              - user clicked Help/About
	cancel_download         - when user clicks Cancel on a download in download_view
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
	bool download_click(GdkEventButton * event);
	void download_file();
	void download_info_tab();
	void download_info_tab_close(gui_download_status * info_scrolled_window, bool * refresh);
	bool download_info_tab_refresh(gui_download_status * info_scrolled_window, bool * refresh);
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

	//holds the information from the last search
	std::vector<download_info> Search_Info;
};
#endif
