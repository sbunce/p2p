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

	//the list which the tree views display
	Glib::RefPtr<Gtk::ListStore> downloadList;
	Glib::RefPtr<Gtk::ListStore> uploadList;
	Glib::RefPtr<Gtk::ListStore> searchList;

	/*functions associated with Gtk signals
	aboutProgram   - user clicked Help/About
	cancelDownload - when user clicks Cancel on a download in downloadsView
	downloadFile   - when user clicks Download on a entry in searchView
	downloadClick  - interprets downloadsView input, can pop up a menu
	quitProgram    - when user clicks File/Quit
	searchInput    - when user clicks searchButton or hits enter while on searchEntry
	searchClick    - interprets searchView input, can pop up a menu
	*/
	void aboutProgram();
	void cancelDownload();
	void downloadFile();
	bool downloadClick(GdkEventButton * event);
	void quitProgram();
	void searchInput();
	bool searchClick(GdkEventButton * event);

	/*
	downloadInfoSetup   - sets up the download tab treeview columns
	downloadInfoRefresh - updates the information in downloadList
	uploadInfoSetup     - sets up the upload tab treeview columns
	uploadInfoRefresh   - updates the information in uploadList
	searchInfoSetup     - sets up the search tab treeview columns
	searchInfoRefresh   - updates the information in searchList
	updateStatusBar     - updates the information in the status bar
	*/
	void downloadInfoSetup();
	bool downloadInfoRefresh();
	void uploadInfoSetup();
	bool uploadInfoRefresh();
	void searchInfoSetup();
	void searchInfoRefresh();
	bool updateStatusBar();

	client Client;
	server Server;
	exploration Exploration;

	//holds the information from the last search
	std::list<exploration::infoBuffer> searchInfo;
};
#endif
