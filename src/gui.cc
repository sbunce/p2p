//std
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "gui.h"
#include "gui_about.h"

gui::gui() : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{
	Server.start();
	Client.start();

	//Gtk objects
	window = this;
	quit = NULL;
	fileMenu = Gtk::manage(new class Gtk::Menu());
	fileMenuItem = NULL;
	about = NULL;
	helpMenu = Gtk::manage(new class Gtk::Menu());
	helpMenuItem = NULL;
	menubar = Gtk::manage(new class Gtk::MenuBar());
	searchEntry = Gtk::manage(new class Gtk::Entry());
	searchButton = Gtk::manage(new class Gtk::Button(("Search")));
	hbox1 = Gtk::manage(new class Gtk::HBox(false, 0));
	searchView = Gtk::manage(new class Gtk::TreeView());
	scrolledwindow1 = Gtk::manage(new class Gtk::ScrolledWindow());
	vbox1 = Gtk::manage(new class Gtk::VBox(false, 0));
	searchTab = Gtk::manage(new class Gtk::Label((" Search ")));
	downloadsView = Gtk::manage(new class Gtk::TreeView());
	scrolledwindow2 = Gtk::manage(new class Gtk::ScrolledWindow());
	downloadsTab = Gtk::manage(new class Gtk::Label((" Downloads ")));
	uploadsView = Gtk::manage(new class Gtk::TreeView());
	scrolledwindow3 = Gtk::manage(new class Gtk::ScrolledWindow());
	uploadsTab = Gtk::manage(new class Gtk::Label((" Uploads ")));
	notebook1 = Gtk::manage(new class Gtk::Notebook());
	statusbar = Gtk::manage(new class Gtk::Statusbar());
	vbox2 = Gtk::manage(new class Gtk::VBox(false, 0));

	//Gtk menu objects
	fileMenu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-quit")));
	quit = (Gtk::ImageMenuItem *)&fileMenu->items().back();
	helpMenu->items().push_back(Gtk::Menu_Helpers::MenuElem(("_About")));
	about = (Gtk::MenuItem *)&helpMenu->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_File"), *fileMenu));
	fileMenuItem = (Gtk::MenuItem *)&menubar->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Help"), *helpMenu));
	helpMenuItem = (Gtk::MenuItem *)&menubar->items().back();

	//Gtk object settings
	searchEntry->set_flags(Gtk::CAN_FOCUS);
	searchEntry->set_visibility(true);
	searchEntry->set_editable(true);
	searchEntry->set_max_length(0);
	searchEntry->set_text((""));
	searchEntry->set_has_frame(true);
	searchEntry->set_activates_default(false);
	searchButton->set_flags(Gtk::CAN_FOCUS);
	searchButton->set_relief(Gtk::RELIEF_NORMAL);
	hbox1->pack_start(*searchEntry);
	hbox1->pack_start(*searchButton, Gtk::PACK_SHRINK, 5);
	searchView->set_flags(Gtk::CAN_FOCUS);
	searchView->set_headers_visible(true);
	searchView->set_rules_hint(false);
	searchView->set_reorderable(false);
	searchView->set_enable_search(true);
	scrolledwindow1->set_flags(Gtk::CAN_FOCUS);
	scrolledwindow1->set_shadow_type(Gtk::SHADOW_IN);
	scrolledwindow1->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	scrolledwindow1->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	scrolledwindow1->add(*searchView);
	vbox1->pack_start(*hbox1, Gtk::PACK_SHRINK, 0);
	vbox1->pack_start(*scrolledwindow1);
	searchTab->set_alignment(0.5,0.5);
	searchTab->set_padding(0,0);
	searchTab->set_justify(Gtk::JUSTIFY_LEFT);
	searchTab->set_line_wrap(false);
	searchTab->set_use_markup(false);
	searchTab->set_selectable(false);
	downloadsView->set_flags(Gtk::CAN_FOCUS);
	downloadsView->set_headers_visible(true);
	downloadsView->set_rules_hint(false);
	downloadsView->set_reorderable(false);
	downloadsView->set_enable_search(true);
	scrolledwindow2->set_flags(Gtk::CAN_FOCUS);
	scrolledwindow2->set_shadow_type(Gtk::SHADOW_IN);
	scrolledwindow2->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	scrolledwindow2->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	scrolledwindow2->add(*downloadsView);
	downloadsTab->set_alignment(0.5,0.5);
	downloadsTab->set_padding(0,0);
	downloadsTab->set_justify(Gtk::JUSTIFY_LEFT);
	downloadsTab->set_line_wrap(false);
	downloadsTab->set_use_markup(false);
	downloadsTab->set_selectable(false);
	uploadsView->set_flags(Gtk::CAN_FOCUS);
	uploadsView->set_headers_visible(true);
	uploadsView->set_rules_hint(false);
	uploadsView->set_reorderable(false);
	uploadsView->set_enable_search(true);
	scrolledwindow3->set_flags(Gtk::CAN_FOCUS);
	scrolledwindow3->set_shadow_type(Gtk::SHADOW_IN);
	scrolledwindow3->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	scrolledwindow3->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	scrolledwindow3->add(*uploadsView);
	uploadsTab->set_alignment(0.5,0.5);
	uploadsTab->set_padding(0,0);
	uploadsTab->set_justify(Gtk::JUSTIFY_LEFT);
	uploadsTab->set_line_wrap(false);
	uploadsTab->set_use_markup(false);
	uploadsTab->set_selectable(false);
	notebook1->set_flags(Gtk::CAN_FOCUS);
	notebook1->set_show_tabs(true);
	notebook1->set_show_border(true);
	notebook1->set_tab_pos(Gtk::POS_TOP);
	notebook1->set_scrollable(false);
	notebook1->append_page(*vbox1, *searchTab);
	notebook1->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook1->append_page(*scrolledwindow2, *downloadsTab);
	notebook1->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook1->append_page(*scrolledwindow3, *uploadsTab);
	notebook1->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	vbox2->pack_start(*menubar, Gtk::PACK_SHRINK, 0);
	vbox2->pack_start(*notebook1);
	vbox2->pack_start(*statusbar, Gtk::PACK_SHRINK, 0);
	window->set_title(global::WINDOW_TITLE);
	window->resize(800, 600);
	window->set_modal(false);
	window->property_window_position().set_value(Gtk::WIN_POS_NONE);
	window->set_resizable(true);
	window->property_destroy_with_parent().set_value(false);
	window->add(*vbox2);
	quit->show();
	fileMenuItem->show();
	about->show();
	helpMenuItem->show();
	menubar->show();
	searchEntry->show();
	searchButton->show();
	hbox1->show();
	searchView->show();
	scrolledwindow1->show();
	vbox1->show();
	searchTab->show();
	downloadsView->show();
	scrolledwindow2->show();
	downloadsTab->show();
	uploadsView->show();
	scrolledwindow3->show();
	uploadsTab->show();
	notebook1->show();
	statusbar->show();
	vbox2->show();
	window->show();

	//Gtk signals
	quit->signal_activate().connect(sigc::mem_fun(*this, &gui::quit_program), false);
	about->signal_activate().connect(sigc::mem_fun(*this, &gui::about_program), false);
	searchEntry->signal_activate().connect(sigc::mem_fun(*this, &gui::search_input), false);
	searchButton->signal_clicked().connect(sigc::mem_fun(*this, &gui::search_input), false);

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
	Client.stop();
}

void gui::about_program()
{
	gui_about * aboutWindow = new gui_about;
	Gtk::Main::run(*aboutWindow);
}

void gui::cancel_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = downloadsView->get_selection();
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

			std::list<exploration::info_buffer>::iterator iter_cur, iter_end;
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
		//without this user would have to left click then right click
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(downloadsView->get_path_at_pos((int)event->x, (int)event->y, path, column, x, y)){
			downloadsView->set_cursor(path);
		}

		return true;
	}

	return false;
}

bool gui::search_click(GdkEventButton * event)
{
	if(event->type == GDK_BUTTON_PRESS && event->button == 3){
		searchPopupMenu.popup(event->button, event->time);

		//select the row when the user right clicks on it
		//without this user would have to left click then right click
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
	downloadsView->set_model(downloadList);

	//add columns
	downloadsView->append_column("  Server IP  ", server_IP_t);
	downloadsView->append_column("  File Name  ", file_name_t);
	downloadsView->append_column("  File Size  ", file_size_t);
	downloadsView->append_column("  Download Speed  ", download_speed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress * cell = new Gtk::CellRendererProgress;
	int cols_count = downloadsView->append_column("  Percent Complete  ", *cell);
	Gtk::TreeViewColumn * pColumn = downloadsView->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percent_complete_t);

	//signal for clicks on downloadsView
	downloadsView->signal_button_press_event().connect(sigc::mem_fun(*this, 
		&gui::download_click), false);

	//menu that pops up when right click happens
	Gtk::Menu::MenuList & menuList = downloadsPopupMenu.items();
	menuList.push_back(Gtk::Menu_Helpers::MenuElem("_Cancel Download",
		sigc::mem_fun(*this, &gui::cancel_download)));
}

bool gui::download_info_refresh()
{
	//update download info
	std::vector<client::info_buffer> info;
	Client.get_download_info(info);

	std::vector<client::info_buffer>::iterator info_iter_cur, info_iter_end;
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
		std::list<std::string>::iterator IP_iter_cur, IP_iter_end;
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
			float file_size = info_iter_cur->file_size / 1024 / 1024;
			std::ostringstream file_size_s;
			if(file_size < 1){
				file_size_s << std::setprecision(2) << file_size << " mB";
			}
			else{
				file_size_s << (int)file_size << " mB";
			}

			row[hash_t] = info_iter_cur->hash;
			row[server_IP_t] = combined_IP;
			row[file_name_t] = info_iter_cur->file_name;
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
		std::vector<client::info_buffer>::iterator info_iter_cur, info_iter_end;
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
		}
		else{
			++Children_iter_cur;
		}
	}

	return true;
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
	uploadsView->set_model(uploadList);

	//add columns
	uploadsView->append_column("  Client IP  ", client_IP_t);
	uploadsView->append_column("  File Name  ", file_name_t);
	uploadsView->append_column("  File Size  ", file_size_t);
	uploadsView->append_column("  Upload Speed  ", uploadSpeed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress* cell = new Gtk::CellRendererProgress;
	int cols_count = uploadsView->append_column("  Percent Complete  ", *cell);
	Gtk::TreeViewColumn* pColumn = uploadsView->get_column(cols_count - 1);
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
			}
			else{
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
			}
			++info_iter_cur;
		}

		//if upload info wasn't found for row delete it
		if(!upload_found){
			Children_iter_cur = downloadList->erase(Children_iter_cur);
		}
		else{
			++Children_iter_cur;
		}
	}

	return true;
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

	//signal for clicks on downloadsView
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

	std::list<exploration::info_buffer>::iterator info_iter_cur, info_iter_end;
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
		float file_size = atof(info_iter_cur->file_size.c_str());
		file_size = file_size / 1024 / 1024;
		std::ostringstream file_size_s;
		if(file_size < 1){
			file_size_s << std::setprecision(2) << file_size << " mB";
		}
		else{
			file_size_s << (int)file_size << " mB";
		}

		//add row
		Gtk::TreeModel::Row row = *(searchList->append());
		row[hash_t] = info_iter_cur->hash;
		row[file_name_t] = info_iter_cur->file_name;
		row[file_size_t] = file_size_s.str();
		row[server_IP_t] = server_IP;

		++info_iter_cur;
	}
}

bool gui::update_status_bar()
{
	std::string status; //holds entire status line

	//get the total client download speed
	int clientSpeed = Client.get_total_speed();
	clientSpeed = clientSpeed / 1024; //convert to kB
	std::ostringstream clientSpeed_s;
	clientSpeed_s << clientSpeed << " kB/s";

	//get the total server upload speed
	int serverSpeed = Server.get_total_speed();
	serverSpeed = serverSpeed / 1024; //convert to kB
	std::ostringstream serverSpeed_s;
	serverSpeed_s << serverSpeed << " kB/s";

	if(Server.is_indexing()){
		status = "  Download Speed: " + clientSpeed_s.str() + " Upload Speed: " + serverSpeed_s.str() + "  Indexing/Hashing ";
	}
	else{
		status = "  Download Speed: " + clientSpeed_s.str() + " Upload Speed: " + serverSpeed_s.str();
	}

	statusbar->pop();
	statusbar->push(status);

	return true;
}

void gui::quit_program()
{
	hide();
}

void gui::search_input()
{
	std::string inputText = searchEntry->get_text();
	searchEntry->set_text("");
	Exploration.search(inputText, Search_Info);
	search_info_refresh();
}
