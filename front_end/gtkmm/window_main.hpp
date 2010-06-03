#ifndef H_WINDOW_MAIN
#define H_WINDOW_MAIN

//custom
#include "settings.hpp"
#include "fixed_statusbar.hpp"
#include "window_about.hpp"
#include "window_prefs.hpp"
#include "window_transfer.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <convert.hpp>
#include <gtkmm.h>
#include <p2p.hpp>

//standard
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

class window_main : public Gtk::Window, private boost::noncopyable
{
public:
	window_main();
	~window_main();

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
	Gtk::Image * download_label;
	Gtk::Image * upload_label;
	Gtk::Image * tracker_label;

	//boxes (divides the window)
	Gtk::VBox * main_VBox; //VBox for the main window

	//bottom bar that displays status etc
	Gtk::Fixed * statusbar;

	//stuff in download/upload tabs
	window_transfer * Window_Download;
	window_transfer * Window_Upload;

	//types of data accepted by drag and drop
	std::list<Gtk::TargetEntry> list_targets;

	/*
	Signaled Functions
	file_drag_data_received:
		File dragged on to window.
	help_about:
		User clicked Help/About.
	on_quit:
		When user clicks File/Quit
	on_delete_event:
		Overrides the event which happens on window close.
	process_URI:
		Processes URI of file dropped on window.
		Note: URI might be newline delimited list of URIs.
	settings_preferences:
		User clicked Settings/Preferences
	*/
	void file_drag_data_received(const Glib::RefPtr<Gdk::DragContext> & context,
		int x, int y, const Gtk::SelectionData & selection_data, guint info, guint time);
	void help_about();
	void on_quit();
	bool on_delete_event(GdkEventAny * event);
	void process_URI(const std::string & URI);
	void settings_preferences();

	p2p P2P;
};
#endif
