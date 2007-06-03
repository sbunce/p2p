//Gtk
#include <gdk/gdkkeysyms.h>
#include <gtkmm/accelgroup.h>
#include <gtk/gtkimagemenuitem.h>

//std
#include <algorithm>
#include <ctime>
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
	window->set_title(("simple-p2p"));
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
	quit->signal_activate().connect(sigc::mem_fun(*this, &gui::quitProgram), false);
	about->signal_activate().connect(sigc::mem_fun(*this, &gui::aboutProgram), false);
	searchEntry->signal_activate().connect(sigc::mem_fun(*this, &gui::searchInput), false);
	searchButton->signal_clicked().connect(sigc::mem_fun(*this, &gui::searchInput), false);

	downloadInfoSetup();
	uploadInfoSetup();
	searchInfoSetup();

	//timed functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::downloadInfoRefresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::uploadInfoRefresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::searchInfoRefresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::updateStatusBar), global::GUI_TICK);
}

void gui::aboutProgram()
{
	gui_about * aboutWindow = new class gui_about;

	Gtk::Main::run(*aboutWindow);
}

void gui::cancelDownload()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = downloadsView->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator iter0 = refSelection->get_selected();

		if(iter0){
			Gtk::TreeModel::Row row = *iter0;

			Glib::ustring messageDigest_retrieved;
			row.get_value(0, messageDigest_retrieved);

			Client.terminateDownload(messageDigest_retrieved);
		}
	}
}

void gui::downloadFile()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = searchView->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator iter0 = refSelection->get_selected();

		if(iter0){
			Gtk::TreeModel::Row row = *iter0;

			Glib::ustring messageDigest_retrieved;
			row.get_value(0, messageDigest_retrieved);

			for(std::vector<exploration::infoBuffer>::iterator iter1 = searchInfo.begin(); iter1 != searchInfo.end(); iter1++){
				if(iter1->messageDigest == messageDigest_retrieved){
					Client.startDownload(*iter1);
					break;
				}
			}


		}
	}
}

bool gui::downloadClick(GdkEventButton * event)
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

bool gui::searchClick(GdkEventButton * event)
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

void gui::downloadInfoSetup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> messageDigest_t;
	Gtk::TreeModelColumn<Glib::ustring> server_IP_t;
	Gtk::TreeModelColumn<Glib::ustring> fileName_t;
	Gtk::TreeModelColumn<Glib::ustring> fileSize_t;
	Gtk::TreeModelColumn<Glib::ustring> downloadSpeed_t;
	Gtk::TreeModelColumn<int> percentComplete_t;

	column.add(messageDigest_t);
	column.add(server_IP_t);
	column.add(fileName_t);
	column.add(fileSize_t);
	column.add(downloadSpeed_t);
	column.add(percentComplete_t);

	downloadList = Gtk::ListStore::create(column);
	downloadsView->set_model(downloadList);

	//add columns
	downloadsView->append_column("  Server IP  ", server_IP_t);
	downloadsView->append_column("  File Name  ", fileName_t);
	downloadsView->append_column("  File Size  ", fileSize_t);
	downloadsView->append_column("  Download Speed  ", downloadSpeed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress * cell = new Gtk::CellRendererProgress;
	int cols_count = downloadsView->append_column("  Percent Complete  ", *cell);
	Gtk::TreeViewColumn * pColumn = downloadsView->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percentComplete_t);

	//signal for clicks on downloadsView
	downloadsView->signal_button_press_event().connect(sigc::mem_fun(*this, 
		&gui::downloadClick), false);

	//menu that pops up when right click happens
	Gtk::Menu::MenuList & menuList = downloadsPopupMenu.items();
	menuList.push_back(Gtk::Menu_Helpers::MenuElem("_Cancel Download",
		sigc::mem_fun(*this, &gui::cancelDownload)));
}

bool gui::downloadInfoRefresh()
{
	//update download info
	std::vector<client::infoBuffer> info;
	Client.getDownloadInfo(info);

	for(std::vector<client::infoBuffer>::iterator iter0 = info.begin(); iter0 != info.end(); iter0++){
		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> messageDigest_t;
		Gtk::TreeModelColumn<Glib::ustring> server_IP_t;
		Gtk::TreeModelColumn<Glib::ustring> fileName_t;
		Gtk::TreeModelColumn<Glib::ustring> fileSize_t;
		Gtk::TreeModelColumn<Glib::ustring> downloadSpeed_t;
		Gtk::TreeModelColumn<int> percentComplete_t;

		column.add(messageDigest_t);
		column.add(server_IP_t);
		column.add(fileName_t);
		column.add(fileSize_t);
		column.add(downloadSpeed_t);
		column.add(percentComplete_t);

		//iterate through all the rows
		bool foundEntry = false;
		Gtk::TreeModel::Children children = downloadList->children();
		for(Gtk::TreeModel::Children::iterator iter1 = children.begin(); iter1 != children.end(); iter1++){
 			Gtk::TreeModel::Row row = *iter1;

			//get the file_ID and fileName to check for existing download
			Glib::ustring messageDigest_retrieved;
			row.get_value(0, messageDigest_retrieved);

			//see if there is already an entry for the file in the download list
			if(messageDigest_retrieved == iter0->messageDigest){
				row[downloadSpeed_t] = iter0->speed;
				row[percentComplete_t] = iter0->percentComplete;

				foundEntry = true;
				break;
			}
		}

		if(!foundEntry){
			Gtk::TreeModel::Row row = *(downloadList->append());

			row[messageDigest_t] = iter0->messageDigest;
			row[server_IP_t] = iter0->server_IP;
			row[fileName_t] = iter0->fileName;
			row[fileSize_t] = iter0->fileSize;
			row[downloadSpeed_t] = iter0->speed;
			row[percentComplete_t] = iter0->percentComplete;
		}
	}

	//get rid of rows without corresponding downloadInfo(finished downloads)
	Gtk::TreeModel::Children children = downloadList->children();

	//if no download info exists at all remove all rows(all downloads complete)
	if(info.size() == 0){
		downloadList->clear();
	}

	for(Gtk::TreeModel::Children::iterator iter0 = children.begin(); iter0 != children.end(); iter0++){
	 	Gtk::TreeModel::Row row = *iter0;

		//get the file_ID and fileName to check for existing download
		Glib::ustring messageDigest_retrieved;
		row.get_value(0, messageDigest_retrieved);

		//loop through the download info and check if we have an entry for download
		bool downloadFound = false;
		for(std::vector<client::infoBuffer>::iterator iter1 = info.begin(); iter1 != info.end(); iter1++){
			//check if server_IP and file_ID match the row
			if(messageDigest_retrieved == iter1->messageDigest){
				downloadFound = true;
			}
		}

		//if download info wasn't found for row delete it(download completed)
		if(!downloadFound){
			downloadList->erase(iter0);
			break;
		}
	}

	return true;
}

void gui::uploadInfoSetup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> client_IP_t;
	Gtk::TreeModelColumn<Glib::ustring> file_ID_t;
	Gtk::TreeModelColumn<Glib::ustring> fileName_t;
	Gtk::TreeModelColumn<Glib::ustring> fileSize_t;
	Gtk::TreeModelColumn<Glib::ustring> uploadSpeed_t;
	Gtk::TreeModelColumn<int> percentComplete_t;

	column.add(client_IP_t);
	column.add(file_ID_t);
	column.add(fileName_t);
	column.add(fileSize_t);
	column.add(uploadSpeed_t);
	column.add(percentComplete_t);

	uploadList = Gtk::ListStore::create(column);
	uploadsView->set_model(uploadList);

	//add columns
	uploadsView->append_column("  Client IP  ", client_IP_t);
	uploadsView->append_column("  File Name  ", fileName_t);
	uploadsView->append_column("  File Size  ", fileSize_t);
	uploadsView->append_column("  Upload Speed  ", uploadSpeed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress* cell = new Gtk::CellRendererProgress;
	int cols_count = uploadsView->append_column("  Percent Complete  ", *cell);
	Gtk::TreeViewColumn* pColumn = uploadsView->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percentComplete_t);
}

bool gui::uploadInfoRefresh()
{
	//update upload info
	std::vector<server::infoBuffer> info;
	Server.getUploadInfo(info);

	for(std::vector<server::infoBuffer>::iterator iter0 = info.begin(); iter0 != info.end(); iter0++){
		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> client_IP_t;
		Gtk::TreeModelColumn<Glib::ustring> file_ID_t;
		Gtk::TreeModelColumn<Glib::ustring> fileName_t;
		Gtk::TreeModelColumn<Glib::ustring> fileSize_t;
		Gtk::TreeModelColumn<Glib::ustring> uploadSpeed_t;
		Gtk::TreeModelColumn<int> percentComplete_t;

		column.add(client_IP_t);
		column.add(file_ID_t);
		column.add(fileName_t);
		column.add(fileSize_t);
		column.add(uploadSpeed_t);
		column.add(percentComplete_t);

		//iterate through all the rows
		bool foundEntry = false;
		Gtk::TreeModel::Children children = uploadList->children();
		for(Gtk::TreeModel::Children::iterator iter1 = children.begin(); iter1 != children.end(); iter1++){
 			Gtk::TreeModel::Row row = *iter1;

			//get the server_IP and file_ID to check for existing download
			Glib::ustring client_IP_retrieved;
			row.get_value(0, client_IP_retrieved);
			Glib::ustring file_ID_retrieved;
			row.get_value(1, file_ID_retrieved);

			//see if there is already an entry for the file in the download list
			if(client_IP_retrieved == iter0->client_IP && file_ID_retrieved == iter0->file_ID){
				row[uploadSpeed_t] = iter0->speed;
				row[percentComplete_t] = iter0->percentComplete;

				foundEntry = true;
				break;
			}
		}

		if(!foundEntry){
			Gtk::TreeModel::Row row = *(uploadList->append());

			row[client_IP_t] = iter0->client_IP;
			row[file_ID_t] = iter0->file_ID;
			row[fileName_t] = iter0->fileName;
			row[fileSize_t] = iter0->fileSize;
			row[uploadSpeed_t] = iter0->speed;
			row[percentComplete_t] = iter0->percentComplete;
		}
	}

	//get rid of rows without corresponding uploadInfo(finished uploads)
	Gtk::TreeModel::Children children = uploadList->children();

	//if no upload info exists at all remove all rows(all uploads complete)
	if(info.size() == 0){
		uploadList->clear();
	}

	for(Gtk::TreeModel::Children::iterator iter0 = children.begin(); iter0 != children.end(); iter0++){
	 	Gtk::TreeModel::Row row = *iter0;

		//get the file_ID and fileName to check for existing download
		Glib::ustring client_IP_retrieved;
		row.get_value(0, client_IP_retrieved);
		Glib::ustring file_ID_retrieved;
		row.get_value(1, file_ID_retrieved);

		//loop through the upload info and check if we have an entry for upload
		bool uploadFound = false;
		for(std::vector<server::infoBuffer>::iterator iter1 = info.begin(); iter1 != info.end(); iter1++){
			//check if server_IP and file_ID match the row
			if(client_IP_retrieved == iter1->client_IP && file_ID_retrieved == iter1->file_ID){
				uploadFound = true;
			}
		}

		//if upload info wasn't found for row delete it(upload completed)
		if(!uploadFound){
			uploadList->erase(iter0);
			break;
		}
	}

	return true;
}

void gui::searchInfoSetup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> messageDigest;
	Gtk::TreeModelColumn<Glib::ustring> fileName_t;
	Gtk::TreeModelColumn<Glib::ustring> fileSize_t;
	Gtk::TreeModelColumn<Glib::ustring> server_IP_t;

	column.add(messageDigest);
	column.add(fileName_t);
	column.add(fileSize_t);
	column.add(server_IP_t);

	searchList = Gtk::ListStore::create(column);
	searchView->set_model(searchList);

	//add columns
	searchView->append_column("  File Name  ", fileName_t);
	searchView->append_column("  File Size  ", fileSize_t);
	searchView->append_column("  Server IP  ", server_IP_t);

	//signal for clicks on downloadsView
	searchView->signal_button_press_event().connect(sigc::mem_fun(*this, &gui::searchClick), false);

	//menu that pops up when right click happens
	Gtk::Menu::MenuList & menuList = searchPopupMenu.items();
	menuList.push_back(Gtk::Menu_Helpers::MenuElem("_Download",
		sigc::mem_fun(*this, &gui::downloadFile)));
}

bool gui::searchInfoRefresh()
{
	//update search info to reflect last search
	Exploration.getSearchResults(searchInfo);

	for(std::vector<exploration::infoBuffer>::iterator iter0 = searchInfo.begin(); iter0 != searchInfo.end(); iter0++){

		std::string server_IP;
		for(std::vector<std::string>::iterator iter1 = iter0->server_IP.begin(); iter1 != iter0->server_IP.end(); iter1++){
			server_IP += *iter1 + ", ";
		}

		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> messageDigest_t;
		Gtk::TreeModelColumn<Glib::ustring> fileName_t;
		Gtk::TreeModelColumn<Glib::ustring> fileSize_t;
		Gtk::TreeModelColumn<Glib::ustring> server_IP_t;

		column.add(messageDigest_t);
		column.add(fileName_t);
		column.add(fileSize_t);
		column.add(server_IP_t);

		//iterate through all the rows
		bool foundEntry = false;
		Gtk::TreeModel::Children children = searchList->children();
		for(Gtk::TreeModel::Children::iterator iter1 = children.begin(); iter1 != children.end(); iter1++){
 			Gtk::TreeModel::Row row = *iter1;

			//get the db_ID to check for existing results
			Glib::ustring messageDigest_retrieved;
			row.get_value(0, messageDigest_retrieved);

			//see if there is already an entry for the file in the download list
			if(messageDigest_retrieved == iter0->messageDigest){
				foundEntry = true;
				break;
			}
		}

		if(!foundEntry){
			Gtk::TreeModel::Row row = *(searchList->append());

			row[messageDigest_t] = iter0->messageDigest;
			row[fileName_t] = iter0->fileName;
			row[fileSize_t] = iter0->fileSize;
			row[server_IP_t] = server_IP;
		}
	}

	//get rid of rows without corresponding searchInfo(user initiated a new search)
	Gtk::TreeModel::Children children = searchList->children();

	//if no info exists in searchList remove all entries
	if(searchInfo.size() == 0){
		searchList->clear();
	}

	for(Gtk::TreeModel::Children::iterator iter0 = children.begin(); iter0 != children.end(); iter0++){
	 	Gtk::TreeModel::Row row = *iter0;

		//get the messageDigest to check for existing results
		Glib::ustring messageDigest_retrieved;
		row.get_value(0, messageDigest_retrieved);

		//loop through the upload info and check if we have an entry for upload
		bool searchInfoFound = false;
		for(std::vector<exploration::infoBuffer>::iterator iter1 = searchInfo.begin(); iter1 != searchInfo.end(); iter1++){
			//check if messageDigest matches row
			if(messageDigest_retrieved == iter1->messageDigest){
				searchInfoFound = true;
			}
		}

		//if upload info wasn't found for row delete it(user initiated a new search)
		if(!searchInfoFound){
			searchList->erase(iter0);
			break;
		}
	}

	return true;
}

bool gui::updateStatusBar()
{
	std::string status;

	if(Server.isIndexing()){
		status = "  Download Speed: " + Client.getTotalSpeed() + " Upload Speed: " + Server.getTotalSpeed() + "  Indexing/Hashing ";
	}
	else{
		status = "  Download Speed: " + Client.getTotalSpeed() + " Upload Speed: " + Server.getTotalSpeed();
	}

	statusbar->pop();
	statusbar->push(status);

	return true;
}

void gui::quitProgram()
{
	hide();
}

void gui::searchInput()
{
	std::string inputText = searchEntry->get_text();
	searchEntry->set_text("");

	Exploration.search(inputText);
}
