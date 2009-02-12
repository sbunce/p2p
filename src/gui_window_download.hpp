#ifndef H_GUI_WINDOW_DOWNLOAD
#define H_GUI_WINDOW_DOWNLOAD

//custom
#include "client.hpp"
#include "global.hpp"
#include "gui_window_download_status.hpp"

//gui
#include <gtkmm.h>

//std
#include <set>
#include <string>

class gui_window_download : public Gtk::ScrolledWindow, private boost::noncopyable
{
public:
	gui_window_download(
		client & Client_in,
		Gtk::Notebook * notebook_in
	);

private:
	//convenience pointer
	Gtk::ScrolledWindow * window;

	//the same notebook that exists in GUI
	Gtk::Notebook * notebook;

	//objects for display of downloads
	Gtk::TreeView * download_view;
	Gtk::ScrolledWindow * download_scrolled_window;
	Glib::RefPtr<Gtk::ListStore> download_list;

	//treeview columns
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> column_hash;
	Gtk::TreeModelColumn<Glib::ustring> column_name;
	Gtk::TreeModelColumn<Glib::ustring> column_size;
	Gtk::TreeModelColumn<Glib::ustring> column_servers;
	Gtk::TreeModelColumn<Glib::ustring> column_speed;
	Gtk::TreeModelColumn<int> column_percent_complete;
	Gtk::CellRendererProgress * cell; //percent renderer

	//popup menus for when user right clicks on treeviews
	Gtk::Menu downloads_popup_menu;

	//hashes of all opened tabs
	std::set<std::string> open_info_tabs;

	//pointer to client that exists in gui class
	client * Client;

	/*
	Signaled Functions
	download_click          - called when download_view clicked
	download_info_tab_close - called when download info tab button clicked
	*/
	bool download_click(GdkEventButton * event);
	void download_info_tab_close(gui_window_download_status * status_window, std::string root_hash, sigc::connection tab_conn);

	/*
	cancel_download       - stop download under mouse cursor
	download_info_tab     - sets up a download_window_download_status tab
	download_info_refresh - refreshes download_view tab
	*/
	void cancel_download();
	void download_info_tab();
	bool download_info_refresh();
};
#endif
