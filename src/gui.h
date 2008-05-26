#ifndef H_GUI
#define H_GUI

//Gtk
#include <gtkmm.h>

//std
#include <list>
#include <string>

//custom
#include "client.h"
#include "download_info.h"
#include "global.h"
#include "server.h"

class gui : public Gtk::Window
{
public:
	gui();
	~gui();

private:
	Gtk::Window * window;     //main window
	Gtk::MenuBar * menubar;   //main menu, holds Gtk::Menu
	Gtk::Notebook * notebook; //main tabs, holds Gtk::Label

	//file menu
	Gtk::Menu * fileMenu;
	Gtk::MenuItem * fileMenuItem;
	Gtk::ImageMenuItem * quit;

	//settings menu
	Gtk::Menu * settingsMenu;
	Gtk::MenuItem * settingsMenuItem;
	Gtk::MenuItem * preferences;

	//help menu
	Gtk::Menu * helpMenu;
	Gtk::MenuItem * helpMenuItem;
	Gtk::MenuItem * about;

	//notebook tabs
	Gtk::Label * searchTab;
	Gtk::Label * downloadTab;
	Gtk::Label * uploadTab;
	Gtk::Label * trackerTab;
	
	//search
	Gtk::Entry * searchEntry;   //input box for searches
	Gtk::Button * searchButton; //search button

	//tracker add
	Gtk::Entry * trackerEntry;   //input box for trackers
	Gtk::Button * trackerButton; //tracker button

	//treeviews for different tabs
	Gtk::TreeView * searchView;
	Gtk::ScrolledWindow * search_scrolledWindow;
	Gtk::TreeView * downloadView;
	Gtk::ScrolledWindow * download_scrolledWindow;
	Gtk::TreeView * uploadView;
	Gtk::ScrolledWindow * upload_scrolledWindow;
	Gtk::TreeView * trackerView;
	Gtk::ScrolledWindow * tracker_scrolledWindow;

	//boxes (divides the window)
	Gtk::HBox * search_HBox;  //separates the search input and search button
	Gtk::HBox * tracker_HBox; //separates the tracker input and add button
	Gtk::VBox * search_VBox;  //VBox which goes inside search_scrolledWindow
	Gtk::VBox * main_VBox;    //VBox for the main window (separates top from bottom)
	Gtk::VBox * tracker_VBox; //VBox which goes inside the tracker_scrolledWindow

	//bottom bar that displays status etc
	Gtk::Statusbar * statusbar;

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloadsPopupMenu;
	Gtk::Menu searchPopupMenu;

	//the lists which the treeviews display
	Glib::RefPtr<Gtk::ListStore> searchList;
	Glib::RefPtr<Gtk::ListStore> downloadList;
	Glib::RefPtr<Gtk::ListStore> uploadList;
	Glib::RefPtr<Gtk::ListStore> trackerList;

	/*functions associated with Gtk signals
	help_about           - user clicked Help/About
	cancel_download      - when user clicks Cancel on a download in downloadView
	download_file        - when user clicks Download on a entry in searchView
	download_click       - interprets downloadView input, can pop up a menu on right click
	on_quit              - when user clicks File/Quit
	on_delete_event      - overrides the event which happens on window close
	search_input         - when user clicks searchButton or hits enter while on searchEntry
	search_click         - interprets searchView input, can pop up a menu on right click
	settings_preferences - user clicked Settings/Preferences
	*/
	void help_about();
	void cancel_download();
	void download_file();
	bool download_click(GdkEventButton * event);
	void on_quit();
	bool on_delete_event(GdkEventAny* event);
	void search_input();
	bool search_click(GdkEventButton * event);
	void settings_preferences();

	/*
	download_info_setup   - sets up the download tab treeview columns
	download_info_refresh - updates the information in downloadList
	upload_info_setup     - sets up the upload tab treeview columns
	upload_info_refresh   - updates the information in uploadList
	search_info_setup     - sets up the search tab treeview columns
	search_info_refresh   - updates the information in searchList
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
