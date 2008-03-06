#ifndef H_GUI
#define H_GUI

//Gtk
#include <gtkmm.h>

//std
#include <list>
#include <string>

//custom
#include "client.h"
#include "DB_access.h"
#include "global.h"
#include "server.h"

class gui : public Gtk::Window
{
public:
	gui();
	~gui();

private:
	class Gtk::Window * window;     //main window
	class Gtk::MenuBar * menubar;   //main menu, holds Gtk::Menu
	class Gtk::Notebook * notebook; //main tabs, holds Gtk::Label

	//file menu
	class Gtk::Menu * fileMenu;
	class Gtk::MenuItem * fileMenuItem;
	class Gtk::ImageMenuItem * quit;

	//help menu
	class Gtk::Menu * helpMenu;
	class Gtk::MenuItem * helpMenuItem;
	class Gtk::MenuItem * about;

	//notebook tabs
	class Gtk::Label * searchTab;
	class Gtk::Label * downloadTab;
	class Gtk::Label * uploadTab;
	class Gtk::Label * trackerTab;
	
	//search
	class Gtk::Entry * searchEntry;   //input box for searches
	class Gtk::Button * searchButton; //search button

	//tracker add
	class Gtk::Entry * trackerEntry;   //input box for trackers
	class Gtk::Button * trackerButton; //tracker button

	//treeviews for different tabs
	class Gtk::TreeView * searchView;
	class Gtk::ScrolledWindow * search_scrolledWindow;
	class Gtk::TreeView * downloadView;
	class Gtk::ScrolledWindow * download_scrolledWindow;
	class Gtk::TreeView * uploadView;
	class Gtk::ScrolledWindow * upload_scrolledWindow;
	class Gtk::TreeView * trackerView;
	class Gtk::ScrolledWindow * tracker_scrolledWindow;

	//boxes (divides the window)
	class Gtk::HBox * search_HBox;  //separates the search input and search button
	class Gtk::HBox * tracker_HBox; //separates the tracker input and add button
	class Gtk::VBox * search_VBox;  //VBox which goes inside search_scrolledWindow
	class Gtk::VBox * main_VBox;    //VBox for the main window (separates top from bottom)
	class Gtk::VBox * tracker_VBox; //VBox which goes inside the tracker_scrolledWindow

	//bottom bar that displays status etc
	class Gtk::Statusbar * statusbar;

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloadsPopupMenu;
	Gtk::Menu searchPopupMenu;

	//the lists which the treeviews display
	Glib::RefPtr<Gtk::ListStore> searchList;
	Glib::RefPtr<Gtk::ListStore> downloadList;
	Glib::RefPtr<Gtk::ListStore> uploadList;
	Glib::RefPtr<Gtk::ListStore> trackerList;

	/*functions associated with Gtk signals
	about_program   - user clicked Help/About
	cancel_download - when user clicks Cancel on a download in downloadView
	download_file   - when user clicks Download on a entry in searchView
	download_click  - interprets downloadView input, can pop up a menu
	quit_program    - when user clicks File/Quit
	search_input    - when user clicks searchButton or hits enter while on searchEntry
	search_click    - interprets searchView input, can pop up a menu
	*/
	void about_program();
	void cancel_download();
	void download_file();
	bool download_click(GdkEventButton * event);
	void quit_program();
	void search_input();
	bool search_click(GdkEventButton * event);

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
	DB_access DB_Access;

	//holds the information from the last search
	std::list<DB_access::download_info_buffer> Search_Info;
};
#endif
