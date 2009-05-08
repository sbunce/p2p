#ifndef H_GUI
#define H_GUI

//boost
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//custom
#include "gui_about.hpp"
#include "gui_statusbar_main.hpp"
#include "gui_vbox_search.hpp"
#include "gui_window_download.hpp"
#include "gui_window_preferences.hpp"
#include "gui_window_upload.hpp"

//p2p
#include <p2p.hpp>

//gui
#include <gtkmm.h>

//include
#include <convert.hpp>

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

	//boxes (divides the window)
	Gtk::VBox * main_VBox; //VBox for the main window

	//bottom bar that displays status etc
	Gtk::Statusbar * statusbar;

	/*
	Custom classes derived from gtkmm objects.
	*/
	gui_vbox_search * GUI_VBox_Search;         //stuff in search tab
	gui_window_download * GUI_Window_Download; //stuff in download tab
	gui_window_upload   * GUI_Window_Upload;   //stuff in upload tab

	/*
	Signaled Functions
	help_about              - user clicked Help/About
	on_quit                 - when user clicks File/Quit
	on_delete_event         - overrides the event which happens on window close
	settings_preferences    - user clicked Settings/Preferences
	*/
	void help_about();
	void on_quit();
	bool on_delete_event(GdkEventAny * event);
	void settings_preferences();

	p2p P2P;
};
#endif
