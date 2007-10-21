#ifndef H_GUI
#define H_GUI

//Gtk
#include <gtkmm.h>

//std
#include <list>
#include <string>

#include "client.h"
#include "global.h"
#include "server.h"
#include "exploration.h"

class gui : public Gtk::Window
{
public:
	gui();
	~gui();

private:
	//Gtk gui objects
	class Gtk::Window * window;
	class Gtk::ImageMenuItem * quit;
	class Gtk::Menu * fileMenu;
	class Gtk::MenuItem * fileMenuItem;
	class Gtk::MenuItem * about;
	class Gtk::Menu * helpMenu;
	class Gtk::MenuItem * helpMenuItem;
	class Gtk::MenuBar * menubar;
	class Gtk::Entry * searchEntry;
	class Gtk::Button * searchButton;
	class Gtk::HBox * hbox1;
	class Gtk::TreeView * searchView;
	class Gtk::ScrolledWindow * scrolledwindow1;
	class Gtk::VBox * vbox1;
	class Gtk::Label * searchTab;
	class Gtk::TreeView * downloadsView;
	class Gtk::ScrolledWindow * scrolledwindow2;
	class Gtk::Label * downloadsTab;
	class Gtk::TreeView * uploadsView;
	class Gtk::ScrolledWindow * scrolledwindow3;
	class Gtk::Label * uploadsTab;
	class Gtk::Notebook * notebook1;
	class Gtk::Statusbar * statusbar;
	class Gtk::VBox * vbox2;

	//popups for when user right clicks on treeviews
	Gtk::Menu downloadsPopupMenu;
	Gtk::Menu searchPopupMenu;

	//the lists which the tree views display
	Glib::RefPtr<Gtk::ListStore> downloadList;
	Glib::RefPtr<Gtk::ListStore> uploadList;
	Glib::RefPtr<Gtk::ListStore> searchList;

	/*functions associated with Gtk signals
	about_program   - user clicked Help/About
	cancel_download - when user clicks Cancel on a download in downloadsView
	download_file   - when user clicks Download on a entry in searchView
	download_click  - interprets downloadsView input, can pop up a menu
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
	exploration Exploration;

	//holds the information from the last search
	std::list<exploration::info_buffer> Search_Info;
};
#endif
