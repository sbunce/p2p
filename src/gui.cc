//std
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

//custom
#include "gui_about.h"
#include "gui_preferences.h"

#include "gui.h"

gui::gui() : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{
	Server.start();
	Client.start();

	window = this;
	menubar = Gtk::manage(new Gtk::MenuBar());
	notebook = Gtk::manage(new Gtk::Notebook());

	//file menu
	fileMenu = Gtk::manage(new Gtk::Menu());
	fileMenuItem = NULL;
	quit = NULL;

	//settings menu
	settingsMenu = Gtk::manage(new Gtk::Menu());
	settingsMenuItem = NULL;
	preferences = NULL;

	//help menu
	helpMenu = Gtk::manage(new Gtk::Menu());
	helpMenuItem = NULL;
	about = NULL;

	//notebook tabs
	searchTab = Gtk::manage(new Gtk::Label((" Search ")));
	downloadTab = Gtk::manage(new Gtk::Label((" Downloads ")));
	uploadTab = Gtk::manage(new Gtk::Label((" Uploads ")));
	trackerTab = Gtk::manage(new Gtk::Label((" Trackers ")));

	//search related
	searchEntry = Gtk::manage(new Gtk::Entry());
	searchButton = Gtk::manage(new Gtk::Button(("Search")));

	//tracker related
	trackerEntry = Gtk::manage(new Gtk::Entry());
	trackerButton = Gtk::manage(new Gtk::Button(("Add")));

	//treeviews for different tabs
	searchView = Gtk::manage(new Gtk::TreeView());
	search_scrolledWindow = Gtk::manage(new Gtk::ScrolledWindow());
	downloadView = Gtk::manage(new Gtk::TreeView());
	download_scrolledWindow = Gtk::manage(new Gtk::ScrolledWindow());
	uploadView = Gtk::manage(new Gtk::TreeView());
	upload_scrolledWindow = Gtk::manage(new Gtk::ScrolledWindow());
	trackerView = Gtk::manage(new Gtk::TreeView());
	tracker_scrolledWindow = Gtk::manage(new Gtk::ScrolledWindow());

	//boxes (divides the window)
	search_HBox = Gtk::manage(new Gtk::HBox(false, 0));
	tracker_HBox = Gtk::manage(new Gtk::HBox(false, 0));
	search_VBox = Gtk::manage(new Gtk::VBox(false, 0));
	main_VBox = Gtk::manage(new Gtk::VBox(false, 0));
	tracker_VBox = Gtk::manage(new Gtk::VBox(false, 0));

	//bottom bar that displays status etc
	statusbar = Gtk::manage(new Gtk::Statusbar());

	//add items to File menu
	fileMenu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-quit")));
	quit = (Gtk::ImageMenuItem *)&fileMenu->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_File"), *fileMenu));
	fileMenuItem = (Gtk::MenuItem *)&menubar->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Settings"), *settingsMenu));
	settingsMenuItem = (Gtk::MenuItem *)&menubar->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Help"), *helpMenu));
	helpMenuItem = (Gtk::MenuItem *)&menubar->items().back();

	//add items to Settings menu
	settingsMenu->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Preferences")));
	preferences = (Gtk::MenuItem *)&settingsMenu->items().back();

	//add items to Help menu
	helpMenu->items().push_back(Gtk::Menu_Helpers::MenuElem(("_About")));
	about = (Gtk::MenuItem *)&helpMenu->items().back();

	//search entry box properties
	searchEntry->set_visibility(true);
	searchEntry->set_editable(true);
	searchEntry->set_max_length(0);
	searchEntry->set_text((""));

	//tracker entry box properties
	trackerEntry->set_visibility(true);
	trackerEntry->set_editable(true);
	trackerEntry->set_max_length(0);
	trackerEntry->set_text((""));

	//add search input/button to horizontal box
	search_HBox->pack_start(*searchEntry);
	search_HBox->pack_start(*searchButton, Gtk::PACK_SHRINK, 5);

	//add tracker input/button to horizontal box
	tracker_HBox->pack_start(*trackerEntry);
	tracker_HBox->pack_start(*trackerButton, Gtk::PACK_SHRINK, 5);

	//TreeView and ScrolledWindow properties
	//search
	searchView->set_flags(Gtk::CAN_FOCUS);
	searchView->set_headers_visible(false); //true enables column labels
	searchView->set_rules_hint(true);       //true sets alternating row background color
	searchView->set_reorderable(false);     //allow moving of TreeView elements
	searchView->set_enable_search(false);   //allow searching of TreeView contents
	search_scrolledWindow->set_flags(Gtk::CAN_FOCUS);
	search_scrolledWindow->set_shadow_type(Gtk::SHADOW_IN);
	search_scrolledWindow->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	search_scrolledWindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	search_scrolledWindow->add(*searchView);
	//download
	downloadView->set_flags(Gtk::CAN_FOCUS);
	downloadView->set_headers_visible(false);
	downloadView->set_rules_hint(true);
	downloadView->set_reorderable(true);
	downloadView->set_enable_search(false);
	download_scrolledWindow->set_flags(Gtk::CAN_FOCUS);
	download_scrolledWindow->set_shadow_type(Gtk::SHADOW_IN);
	download_scrolledWindow->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	download_scrolledWindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	download_scrolledWindow->add(*downloadView);
	//upload
	uploadView->set_flags(Gtk::CAN_FOCUS);
	uploadView->set_headers_visible(false);
	uploadView->set_rules_hint(true);
	uploadView->set_reorderable(false);
	uploadView->set_enable_search(false);
	upload_scrolledWindow->set_flags(Gtk::CAN_FOCUS);
	upload_scrolledWindow->set_shadow_type(Gtk::SHADOW_IN);
	upload_scrolledWindow->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	upload_scrolledWindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	upload_scrolledWindow->add(*uploadView);
	//tracker
	trackerView->set_flags(Gtk::CAN_FOCUS);
	trackerView->set_headers_visible(false);
	trackerView->set_rules_hint(true);
	trackerView->set_reorderable(true);
	trackerView->set_enable_search(false);
	tracker_scrolledWindow->set_flags(Gtk::CAN_FOCUS);
	tracker_scrolledWindow->set_shadow_type(Gtk::SHADOW_IN);
	tracker_scrolledWindow->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	tracker_scrolledWindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	tracker_scrolledWindow->add(*trackerView);

	//add search input/button/TreeView to the window for searching
	search_VBox->pack_start(*search_HBox, Gtk::PACK_SHRINK, 0);
	search_VBox->pack_start(*search_scrolledWindow);

	//add tracker input/button/TreeView to the window for adding trackers
	tracker_VBox->pack_start(*tracker_HBox, Gtk::PACK_SHRINK, 0);
	tracker_VBox->pack_start(*tracker_scrolledWindow);

	//tab properties
	//search
	searchTab->set_justify(Gtk::JUSTIFY_LEFT);
	searchTab->set_line_wrap(false);  //allows wrapping of tab label
	searchTab->set_selectable(false); //true allows selecting of the text from the label
	//download
	downloadTab->set_justify(Gtk::JUSTIFY_LEFT);
	downloadTab->set_line_wrap(false);
	downloadTab->set_selectable(false);
	//upload
	uploadTab->set_justify(Gtk::JUSTIFY_LEFT);
	uploadTab->set_line_wrap(false);
	uploadTab->set_selectable(false);
	//tracker
	trackerTab->set_justify(Gtk::JUSTIFY_LEFT);
	trackerTab->set_line_wrap(false);
	trackerTab->set_selectable(false);

	//set notebook properties
	notebook->set_flags(Gtk::CAN_FOCUS);
	notebook->set_show_tabs(true);
	notebook->set_show_border(true);
	notebook->set_tab_pos(Gtk::POS_TOP);
	notebook->set_scrollable(false);

	//add elements to the notebook
	notebook->append_page(*search_VBox, *searchTab);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook->append_page(*download_scrolledWindow, *downloadTab);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook->append_page(*upload_scrolledWindow, *uploadTab);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook->append_page(*tracker_VBox, *trackerTab);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);

	//add items to the main VBox
	main_VBox->pack_start(*menubar, Gtk::PACK_SHRINK, 0);
	main_VBox->pack_start(*notebook);
	main_VBox->pack_start(*statusbar, Gtk::PACK_SHRINK, 0);

	//window properties
	window->set_title(global::NAME);
	window->resize(800, 600);
	window->set_modal(false);
	window->property_window_position().set_value(Gtk::WIN_POS_NONE);
	window->set_resizable(true);
	window->property_destroy_with_parent().set_value(false);
	window->add(*main_VBox);

	//set objects to be visible
	show_all_children();

	//signaled functions
	quit->signal_activate().connect(sigc::mem_fun(*this, &gui::file_quit), false);
	preferences->signal_activate().connect(sigc::mem_fun(*this, &gui::settings_preferences), false);
	about->signal_activate().connect(sigc::mem_fun(*this, &gui::help_about), false);
	searchEntry->signal_activate().connect(sigc::mem_fun(*this, &gui::search_input), false);
	searchButton->signal_clicked().connect(sigc::mem_fun(*this, &gui::search_input), false);

	//set up Gtk::TreeView for each tab
	download_info_setup();
	upload_info_setup();
	search_info_setup();

	//timed functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::download_info_refresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::upload_info_refresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::update_status_bar), global::GUI_TICK);
}

gui::~gui()
{
	//stop all client and server threads before terminating
	Client.stop();
	Server.stop();
}

void gui::cancel_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = downloadView->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator iter0 = refSelection->get_selected();
		if(iter0){
			Gtk::TreeModel::Row row = *iter0;
			Glib::ustring messageDigest_retrieved;
			row.get_value(0, messageDigest_retrieved);
			Client.stop_download(messageDigest_retrieved);
		}
	}
}

void gui::download_file()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = searchView->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Gtk::TreeModel::Row row = *selected_row_iter;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);

			std::vector<download_info>::iterator iter_cur, iter_end;
			iter_cur = Search_Info.begin();
			iter_end = Search_Info.end();
			while(iter_cur != iter_end){
				if(iter_cur->hash == hash_retrieved){
					Client.start_download(*iter_cur);
					break;
				}
				++iter_cur;
			}
		}
	}
}

bool gui::download_click(GdkEventButton * event)
{
	if(event->type == GDK_BUTTON_PRESS && event->button == 3){
		downloadsPopupMenu.popup(event->button, event->time);

		//select the row when the user right clicks on it
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(downloadView->get_path_at_pos((int)event->x, (int)event->y, path, column, x, y)){
			downloadView->set_cursor(path);
		}
		return true;
	}
	return false;
}

void gui::download_info_setup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> messageDigest_t;
	Gtk::TreeModelColumn<Glib::ustring> server_IP_t;
	Gtk::TreeModelColumn<Glib::ustring> file_name_t;
	Gtk::TreeModelColumn<Glib::ustring> file_size_t;
	Gtk::TreeModelColumn<Glib::ustring> download_speed_t;
	Gtk::TreeModelColumn<int> percent_complete_t;

	column.add(messageDigest_t);
	column.add(server_IP_t);
	column.add(file_name_t);
	column.add(file_size_t);
	column.add(download_speed_t);
	column.add(percent_complete_t);

	downloadList = Gtk::ListStore::create(column);
	downloadView->set_model(downloadList);

	//add columns
	downloadView->append_column("  Server IP  ", server_IP_t);
	downloadView->append_column("  File Name  ", file_name_t);
	downloadView->append_column("  File Size  ", file_size_t);
	downloadView->append_column("  Download Speed  ", download_speed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress * cell = new Gtk::CellRendererProgress;
	int cols_count = downloadView->append_column("  Percent Complete  ", *cell);
	Gtk::TreeViewColumn * pColumn = downloadView->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percent_complete_t);

	//signal for clicks on downloadView
	downloadView->signal_button_press_event().connect(sigc::mem_fun(*this, 
		&gui::download_click), false);

	//menu that pops up when right click happens
	Gtk::Menu::MenuList & menuList = downloadsPopupMenu.items();
	menuList.push_back(Gtk::Menu_Helpers::MenuElem("_Cancel Download",
		sigc::mem_fun(*this, &gui::cancel_download)));
}

bool gui::download_info_refresh()
{
	//update download info
	std::vector<download_info> info;
	Client.current_downloads(info);

	std::vector<download_info>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = info.begin();
	info_iter_end = info.end();
	while(info_iter_cur != info_iter_end){
		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> hash_t;
		Gtk::TreeModelColumn<Glib::ustring> server_IP_t;
		Gtk::TreeModelColumn<Glib::ustring> file_name_t;
		Gtk::TreeModelColumn<Glib::ustring> file_size_t;
		Gtk::TreeModelColumn<Glib::ustring> download_speed_t;
		Gtk::TreeModelColumn<int> percent_complete_t;

		column.add(hash_t);
		column.add(server_IP_t);
		column.add(file_name_t);
		column.add(file_size_t);
		column.add(download_speed_t);
		column.add(percent_complete_t);

		//get all the server_IP information in one string
		std::string combined_IP;
		std::vector<std::string>::iterator IP_iter_cur, IP_iter_end;
		IP_iter_cur = info_iter_cur->server_IP.begin();
		IP_iter_end = info_iter_cur->server_IP.end();
		while(IP_iter_cur != IP_iter_end){
			combined_IP += *IP_iter_cur + "|";
			++IP_iter_cur;
		}

		//convert the download speed to kiloBytes
		int downloadSpeed = info_iter_cur->speed / 1024;
		std::ostringstream downloadSpeed_s;
		downloadSpeed_s << downloadSpeed << " kB/s";

		//iterate through all the rows
		bool foundEntry = false;
		Gtk::TreeModel::Children children = downloadList->children();

		Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
		Children_iter_cur = children.begin();
		Children_iter_end = children.end();
		while(Children_iter_cur != Children_iter_end){
 			Gtk::TreeModel::Row row = *Children_iter_cur;

			//get the hash for comparison purposes
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);

			//see if there is already an entry for the file in the download list
			if(hash_retrieved == info_iter_cur->hash){
				row[server_IP_t] = combined_IP;
				row[download_speed_t] = downloadSpeed_s.str();
				row[percent_complete_t] = info_iter_cur->percent_complete;
				foundEntry = true;
				break;
			}
			++Children_iter_cur;
		}

		if(!foundEntry){
			Gtk::TreeModel::Row row = *(downloadList->append());

			//convert file_size from B to MB
			float file_size = info_iter_cur->size / 1024 / 1024;
			std::ostringstream file_size_s;
			if(file_size < 1){
				file_size_s << std::setprecision(2) << file_size << " mB";
			}else{
				file_size_s << (int)file_size << " mB";
			}

			row[hash_t] = info_iter_cur->hash;
			row[server_IP_t] = combined_IP;
			row[file_name_t] = info_iter_cur->name;
			row[file_size_t] = file_size_s.str();
			row[download_speed_t] = downloadSpeed_s.str();
			row[percent_complete_t] = info_iter_cur->percent_complete;
		}

		++info_iter_cur;
	}

	//get rid of rows without corresponding downloadInfo
	Gtk::TreeModel::Children children = downloadList->children();

	//if no download info exists at all remove all rows
	if(info.size() == 0){
		downloadList->clear();
	}

	Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
	Children_iter_cur = children.begin();
	Children_iter_end = children.end();
	while(Children_iter_cur != Children_iter_end){
	 	Gtk::TreeModel::Row row = *Children_iter_cur;

		//get the hash for comparison purposes
		Glib::ustring hash_retrieved;
		row.get_value(0, hash_retrieved);

		//loop through the download info and check if we have an entry for download
		std::vector<download_info>::iterator info_iter_cur, info_iter_end;
		info_iter_cur = info.begin();
		info_iter_end = info.end();
		bool downloadFound = false;
		while(info_iter_cur != info_iter_end){
			if(hash_retrieved == info_iter_cur->hash){
				downloadFound = true;
				break;
			}
			++info_iter_cur;
		}

		//if download info wasn't found for row delete it
		if(!downloadFound){
			Children_iter_cur = downloadList->erase(Children_iter_cur);
		}else{
			++Children_iter_cur;
		}
	}

	return true;
}

void gui::file_quit()
{
	hide();
}

void gui::help_about()
{
	gui_about * aboutWindow = new gui_about;
	Gtk::Main::run(*aboutWindow);
}

void gui::upload_info_setup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> client_IP_t;
	Gtk::TreeModelColumn<Glib::ustring> file_ID_t;
	Gtk::TreeModelColumn<Glib::ustring> file_name_t;
	Gtk::TreeModelColumn<Glib::ustring> file_size_t;
	Gtk::TreeModelColumn<Glib::ustring> uploadSpeed_t;
	Gtk::TreeModelColumn<int> percent_complete_t;

	column.add(client_IP_t);
	column.add(file_ID_t);
	column.add(file_name_t);
	column.add(file_size_t);
	column.add(uploadSpeed_t);
	column.add(percent_complete_t);

	uploadList = Gtk::ListStore::create(column);
	uploadView->set_model(uploadList);

	//add columns
	uploadView->append_column("  Client IP  ", client_IP_t);
	uploadView->append_column("  File Name  ", file_name_t);
	uploadView->append_column("  File Size  ", file_size_t);
	uploadView->append_column("  Upload Speed  ", uploadSpeed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress* cell = new Gtk::CellRendererProgress;
	int cols_count = uploadView->append_column("  Percent Complete  ", *cell);
	Gtk::TreeViewColumn* pColumn = uploadView->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percent_complete_t);
}

bool gui::upload_info_refresh()
{
	//update upload info
	std::vector<server::info_buffer> info;
	Server.get_upload_info(info);

	std::vector<server::info_buffer>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = info.begin();
	info_iter_end = info.end();
	while(info_iter_cur != info_iter_end){
		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> client_IP_t;
		Gtk::TreeModelColumn<Glib::ustring> file_ID_t;
		Gtk::TreeModelColumn<Glib::ustring> file_name_t;
		Gtk::TreeModelColumn<Glib::ustring> file_size_t;
		Gtk::TreeModelColumn<Glib::ustring> uploadSpeed_t;
		Gtk::TreeModelColumn<int> percent_complete_t;

		column.add(client_IP_t);
		column.add(file_ID_t);
		column.add(file_name_t);
		column.add(file_size_t);
		column.add(uploadSpeed_t);
		column.add(percent_complete_t);

		std::ostringstream speed_s;
		speed_s << (info_iter_cur->speed / 1024) << " kB/s";

		//iterate through all the rows
		Gtk::TreeModel::Children children = uploadList->children();
		Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
		Children_iter_cur = children.begin();
		Children_iter_end = children.end();
		bool foundEntry = false;
		while(Children_iter_cur != Children_iter_end){
 			Gtk::TreeModel::Row row = *Children_iter_cur;

			//get the server_IP and file_ID to check for existing download
			Glib::ustring client_IP_retrieved;
			row.get_value(0, client_IP_retrieved);
			Glib::ustring file_ID_retrieved;
			row.get_value(1, file_ID_retrieved);

			//see if there is already an entry for the file in the download list
			if(client_IP_retrieved == info_iter_cur->client_IP && file_ID_retrieved == info_iter_cur->file_ID){
				row[uploadSpeed_t] = speed_s.str();
				row[percent_complete_t] = info_iter_cur->percent_complete;
				foundEntry = true;
				break;
			}

			++Children_iter_cur;
		}

		if(!foundEntry){
			Gtk::TreeModel::Row row = *(uploadList->append());

			//change display to a decimal if less than 1mB
			float file_size = info_iter_cur->file_size / 1024 / 1024;
			std::ostringstream file_size_s;
			if(file_size < 1){
				file_size_s << std::setprecision(2) << file_size << " mB";
			}else{
				file_size_s << (int)file_size << " mB";
			}

			row[client_IP_t] = info_iter_cur->client_IP;
			row[file_ID_t] = info_iter_cur->file_ID;
			row[file_name_t] = info_iter_cur->file_name;
			row[file_size_t] = file_size_s.str();
			row[uploadSpeed_t] = speed_s.str();
			row[percent_complete_t] = info_iter_cur->percent_complete;
		}

		++info_iter_cur;
	}

	//get rid of rows without corresponding uploadInfo(finished uploads)
	Gtk::TreeModel::Children children = uploadList->children();

	//if no upload info exists at all remove all rows(all uploads complete)
	if(info.size() == 0){
		uploadList->clear();
	}

	Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
	Children_iter_cur = children.begin();
	Children_iter_end = children.end();
	while(Children_iter_cur != Children_iter_end){
	 	Gtk::TreeModel::Row row = *Children_iter_cur;

		//get the file_ID and file_name to check for existing download
		Glib::ustring client_IP_retrieved;
		row.get_value(0, client_IP_retrieved);
		Glib::ustring file_ID_retrieved;
		row.get_value(1, file_ID_retrieved);

		//loop through the upload info and check if we have an entry for upload
		std::vector<server::info_buffer>::iterator info_iter_cur, info_iter_end;
		info_iter_cur = info.begin();
		info_iter_end = info.end();
		bool upload_found = false;
		while(info_iter_cur != info_iter_end){
			//check if server_IP and file_ID match the row
			if(client_IP_retrieved == info_iter_cur->client_IP && file_ID_retrieved == info_iter_cur->file_ID){
				upload_found = true;
				break;
			}
			++info_iter_cur;
		}

		//if upload info wasn't found for row delete it
		if(!upload_found){
			Children_iter_cur = uploadList->erase(Children_iter_cur);
		}else{
			++Children_iter_cur;
		}
	}

	return true;
}

bool gui::search_click(GdkEventButton * event)
{
	if(event->type == GDK_BUTTON_PRESS && event->button == 3){
		searchPopupMenu.popup(event->button, event->time);

		//select the row when the user right clicks on it
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(searchView->get_path_at_pos((int)event->x, (int)event->y, path, column, x, y)){
			searchView->set_cursor(path);
		}
		return true;
	}
	return false;
}

void gui::search_info_setup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> messageDigest;
	Gtk::TreeModelColumn<Glib::ustring> file_name_t;
	Gtk::TreeModelColumn<Glib::ustring> file_size_t;
	Gtk::TreeModelColumn<Glib::ustring> server_IP_t;

	column.add(messageDigest);
	column.add(file_name_t);
	column.add(file_size_t);
	column.add(server_IP_t);

	searchList = Gtk::ListStore::create(column);
	searchView->set_model(searchList);

	//add columns
	searchView->append_column("  File Name  ", file_name_t);
	searchView->append_column("  File Size  ", file_size_t);
	searchView->append_column("  Server IP  ", server_IP_t);

	//signal for clicks on downloadView
	searchView->signal_button_press_event().connect(sigc::mem_fun(*this, &gui::search_click), false);

	//menu that pops up when right click happens
	Gtk::Menu::MenuList & menuList = searchPopupMenu.items();
	menuList.push_back(Gtk::Menu_Helpers::MenuElem("_Download",
		sigc::mem_fun(*this, &gui::download_file)));
}

void gui::search_info_refresh()
{
	//clear all results
	searchList->clear();

	std::vector<download_info>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = Search_Info.begin();
	info_iter_end = Search_Info.end();
	while(info_iter_cur != info_iter_end){

		std::vector<std::string>::iterator IP_iter_cur, IP_iter_end;
		IP_iter_cur = info_iter_cur->server_IP.begin();
		IP_iter_end = info_iter_cur->server_IP.end();
		std::string server_IP;
		while(IP_iter_cur != IP_iter_end){
			server_IP += *IP_iter_cur + "|";
			++IP_iter_cur;
		}

		//set up columns
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> hash_t;
		Gtk::TreeModelColumn<Glib::ustring> file_name_t;
		Gtk::TreeModelColumn<Glib::ustring> file_size_t;
		Gtk::TreeModelColumn<Glib::ustring> server_IP_t;
		column.add(hash_t);
		column.add(file_name_t);
		column.add(file_size_t);
		column.add(server_IP_t);

		//convert file_size from Bytes to megaBytes
		float file_size = info_iter_cur->size;
		file_size = file_size / 1024 / 1024;
		std::ostringstream file_size_s;
		if(file_size < 1){
			file_size_s << std::setprecision(2) << file_size << " mB";
		}else{
			file_size_s << (int)file_size << " mB";
		}

		//add row
		Gtk::TreeModel::Row row = *(searchList->append());
		row[hash_t] = info_iter_cur->hash;
		row[file_name_t] = info_iter_cur->name;
		row[file_size_t] = file_size_s.str();
		row[server_IP_t] = server_IP;

		++info_iter_cur;
	}
}

void gui::search_input()
{
	std::string input_text = searchEntry->get_text();
	searchEntry->set_text("");
	Client.search(input_text, Search_Info);
	search_info_refresh();
}

void gui::settings_preferences()
{
	gui_preferences * preferencesWindow = new gui_preferences;
	Gtk::Main::run(*preferencesWindow);
}

bool gui::update_status_bar()
{
	std::string status; //holds entire status line

	//get the total client download speed
	int clientSpeed = Client.total_speed();
	clientSpeed = clientSpeed / 1024; //convert to kB
	std::ostringstream clientSpeed_s;
	clientSpeed_s << clientSpeed << " kB/s";

	//get the total server upload speed
	int serverSpeed = Server.get_total_speed();
	serverSpeed = serverSpeed / 1024; //convert to kB
	std::ostringstream serverSpeed_s;
	serverSpeed_s << serverSpeed << " kB/s";

	if(Server.is_indexing()){
		status = "  D: " + clientSpeed_s.str() + " U: " + serverSpeed_s.str() + "  Hashing ";
	}else{
		status = "  D: " + clientSpeed_s.str() + " U: " + serverSpeed_s.str();
	}

	statusbar->pop();
	statusbar->push(status);

	return true;
}
