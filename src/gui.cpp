#include "gui.hpp"

gui::gui() : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{
//DEBUG, remove this
	window = this;

	menubar = Gtk::manage(new Gtk::MenuBar);
	notebook = Gtk::manage(new Gtk::Notebook);

	//menus
	file_menu = Gtk::manage(new Gtk::Menu);
	settings_menu = Gtk::manage(new Gtk::Menu);
	help_menu = Gtk::manage(new Gtk::Menu);

	//notebook labels
	Gtk::Image * search_label = Gtk::manage(new Gtk::Image(Gtk::Stock::FIND, Gtk::ICON_SIZE_LARGE_TOOLBAR));
	Gtk::Image * download_label = Gtk::manage(new Gtk::Image(Gtk::Stock::GO_DOWN, Gtk::ICON_SIZE_LARGE_TOOLBAR));
	Gtk::Image * upload_label = Gtk::manage(new Gtk::Image(Gtk::Stock::GO_UP, Gtk::ICON_SIZE_LARGE_TOOLBAR));

//DEUBG, add documentation
	GUI_VBox_Search = Gtk::manage(new gui_vbox_search(&Client));

	//treeviews for different tabs
	download_view = Gtk::manage(new Gtk::TreeView);
	download_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	upload_view = Gtk::manage(new Gtk::TreeView);
	upload_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);

	//boxes (divides the window)
	main_VBox = Gtk::manage(new Gtk::VBox(false, 0));

	//bottom bar that displays status etc
	statusbar = Gtk::manage(new Gtk::Statusbar);

	//add items to File menu
	file_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::QUIT)));
	quit = (Gtk::ImageMenuItem *)&file_menu->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_File"), *file_menu));
	file_menu_item = (Gtk::MenuItem *)&menubar->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Settings"), *settings_menu));
	settings_menu_item = (Gtk::MenuItem *)&menubar->items().back();
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Help"), *help_menu));
	help_menu_item = (Gtk::MenuItem *)&menubar->items().back();

	//add items to Settings menu
	settings_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::PREFERENCES)));
	preferences = (Gtk::MenuItem *)&settings_menu->items().back();

	//add items to Help menu
	help_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::ABOUT)));
	about = (Gtk::MenuItem *)&help_menu->items().back();

	//TreeView and ScrolledWindow properties
	//download
	download_view->set_headers_visible(true);
	download_view->set_rules_hint(true);
	download_view->set_enable_search(false);
	download_scrolled_window->add(*download_view);
	//upload
	upload_view->set_headers_visible(true);
	upload_view->set_rules_hint(true);
	upload_view->set_enable_search(false);
	upload_scrolled_window->add(*upload_view);

	//set notebook properties
	notebook->set_flags(Gtk::CAN_FOCUS);
	notebook->set_show_tabs(true);
	notebook->set_show_border(true);
	notebook->set_tab_pos(Gtk::POS_TOP);
	notebook->set_scrollable(true);

	//add elements to the notebook
	notebook->append_page(*GUI_VBox_Search, *search_label);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook->append_page(*download_scrolled_window, *download_label);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook->append_page(*upload_scrolled_window, *upload_label);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);

	//add items to the main VBox
	main_VBox->pack_start(*menubar, Gtk::PACK_SHRINK, 0);
	main_VBox->pack_start(*notebook);
	main_VBox->pack_start(*statusbar, Gtk::PACK_SHRINK, 0);

	//window properties
	window->set_title(global::NAME);
	window->resize(750, 550);
	window->set_modal(false);
	window->property_window_position().set_value(Gtk::WIN_POS_CENTER_ON_PARENT);
	window->set_resizable(true);
	window->property_destroy_with_parent().set_value(false);
	window->add(*main_VBox);

	//set objects to be visible
	show_all_children();

	//signaled functions
	quit->signal_activate().connect(sigc::mem_fun(*this, &gui::on_quit), false);
	preferences->signal_activate().connect(sigc::mem_fun(*this, &gui::settings_preferences), false);
	about->signal_activate().connect(sigc::mem_fun(*this, &gui::help_about), false);

	//set up Gtk::TreeView for each tab
	download_info_setup();
	upload_info_setup();

	//timed functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::download_info_refresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::upload_info_refresh), global::GUI_TICK);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui::update_status_bar), global::GUI_TICK);
}

gui::~gui()
{

}

void gui::on_quit()
{
	hide();
}

bool gui::on_delete_event(GdkEventAny* event)
{
	on_quit();
	return true;
}

void gui::cancel_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator iter0 = refSelection->get_selected();
		if(iter0){
			Gtk::TreeModel::Row row = *iter0;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			Client.stop_download(hash_retrieved);
		}
	}
}

bool gui::download_click(GdkEventButton * event)
{
	if(event->type == GDK_BUTTON_PRESS && event->button == 1){
		//left click
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(download_view->get_path_at_pos((int)event->x, (int)event->y, path, column, x, y)){
			download_view->set_cursor(path);
		}else{
			download_view->get_selection()->unselect_all();
		}
	}else if(event->type == GDK_BUTTON_PRESS && event->button == 3){
		//right click
		downloads_popup_menu.popup(event->button, event->time);
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(download_view->get_path_at_pos((int)event->x, (int)event->y, path, column, x, y)){
			download_view->set_cursor(path);
		}else{
			download_view->get_selection()->unselect_all();
		}
	}
	return true;
}

void gui::download_info_tab()
{
	std::string root_hash, file_name;
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Gtk::TreeModel::Row row = *selected_row_iter;
			Glib::ustring hash_retrieved;
			row.get_value(0, root_hash);
		}
	}

	//make sure download exists (user could have requested information on nothing)
	std::string name;
	boost::uint64_t tree_size, file_size;
	if(!Client.file_info(root_hash, name, tree_size, file_size)){
		return;
	}

	//objects needed for new tab
	Gtk::HBox * hbox = Gtk::manage(new Gtk::HBox(false, 0));
	Gtk::Label * tab_label = Gtk::manage(new Gtk::Label);
	Gtk::Button * close_button = Gtk::manage(new Gtk::Button);
	Gtk::Image * close_image = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU));
	gui_window_download_status * status_window = Gtk::manage(new gui_window_download_status(root_hash, tab_label, &Client));

	hbox->pack_start(*tab_label);
	close_button->add(*close_image);
	hbox->pack_start(*close_button);
	notebook->append_page(*status_window, *hbox, *tab_label);
	hbox->show_all_children();
	show_all_children();

	//used to tell the refresh function to stop when tab closed
	boost::shared_ptr<bool> refresh(new bool(true));

	//set tab window to refresh
	sigc::connection tab_conn = Glib::signal_timeout().connect(
		sigc::mem_fun(status_window, &gui_window_download_status::refresh),
		global::GUI_TICK
	);

	//signal to close tab, function pointer bound to window pointer associated with a tab
	close_button->signal_clicked().connect(
		sigc::bind<gui_window_download_status *, sigc::connection>(
			sigc::mem_fun(*this, &gui::download_info_tab_close),
			status_window, tab_conn
		)
	);
}

void gui::download_info_tab_close(gui_window_download_status * status_window, sigc::connection tab_conn)
{
	//stop window from refreshing
	tab_conn.disconnect();

	//remove tab
	notebook->remove_page(*status_window);
}

void gui::download_info_setup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> hash_t;
	Gtk::TreeModelColumn<Glib::ustring> name_t;
	Gtk::TreeModelColumn<Glib::ustring> size_t;
	Gtk::TreeModelColumn<Glib::ustring> servers_t;
	Gtk::TreeModelColumn<Glib::ustring> speed_t;
	Gtk::TreeModelColumn<int> percent_complete_t;

	column.add(hash_t);
	column.add(name_t);
	column.add(size_t);
	column.add(servers_t);
	column.add(speed_t);
	column.add(percent_complete_t);

	download_list = Gtk::ListStore::create(column);
	download_view->set_model(download_list);

	//add columns
	download_view->append_column(" Name ", name_t);
	download_view->append_column(" Size ", size_t);
	download_view->append_column(" Peers ", servers_t);
	download_view->append_column(" Speed ", speed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress * cell = new Gtk::CellRendererProgress;
	int cols_count = download_view->append_column(" Complete ", *cell);
	Gtk::TreeViewColumn * pColumn = download_view->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percent_complete_t);

	//signal for clicks on download_view
	download_view->signal_button_press_event().connect(sigc::mem_fun(*this, &gui::download_click), false);

	//menu that pops up when right click happens on download
	Gtk::Menu::MenuList & menu_list = downloads_popup_menu.items();

	menu_list.push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::CANCEL), sigc::mem_fun(*this, &gui::cancel_download)));
	menu_list.push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::INFO), sigc::mem_fun(*this, &gui::download_info_tab)));
}

bool gui::download_info_refresh()
{
	//update download info
	std::vector<download_status> info;
	Client.current_downloads(info);

	std::vector<download_status>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = info.begin();
	info_iter_end = info.end();
	while(info_iter_cur != info_iter_end){
		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> hash_t;
		Gtk::TreeModelColumn<Glib::ustring> name_t;
		Gtk::TreeModelColumn<Glib::ustring> servers_t;
		Gtk::TreeModelColumn<Glib::ustring> size_t;
		Gtk::TreeModelColumn<Glib::ustring> speed_t;
		Gtk::TreeModelColumn<int> percent_complete_t;

		column.add(hash_t);
		column.add(name_t);
		column.add(size_t);
		column.add(servers_t);
		column.add(speed_t);
		column.add(percent_complete_t);

		std::stringstream ss;
		ss << info_iter_cur->IP.size();
		std::string speed = convert::size_unit_select(info_iter_cur->total_speed) + "/s";
		std::string size = convert::size_unit_select(info_iter_cur->size);

		//update rows
		bool entry_found = false;
		Gtk::TreeModel::Children children = download_list->children();
		Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
		Children_iter_cur = children.begin();
		Children_iter_end = children.end();
		while(Children_iter_cur != Children_iter_end){
 			Gtk::TreeModel::Row row = *Children_iter_cur;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			if(hash_retrieved == info_iter_cur->hash){
				row[name_t] = info_iter_cur->name;
				row[size_t] = size;
				row[servers_t] = ss.str();
				row[speed_t] = speed;
				row[percent_complete_t] = info_iter_cur->percent_complete;
				entry_found = true;
				break;
			}
			++Children_iter_cur;
		}

		if(!entry_found){
			Gtk::TreeModel::Row row = *(download_list->append());
			row[hash_t] = info_iter_cur->hash;
			row[name_t] = info_iter_cur->name;
			row[size_t] = size;
			row[servers_t] = ss.str();
			row[speed_t] = speed;
			row[percent_complete_t] = info_iter_cur->percent_complete;
		}
		++info_iter_cur;
	}

	//if no download info exists remove all remaining rows
	if(info.size() == 0){
		download_list->clear();
	}

	//remove rows without corresponding download_info
	Gtk::TreeModel::Children children = download_list->children();
	Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
	Children_iter_cur = children.begin();
	Children_iter_end = children.end();
	while(Children_iter_cur != Children_iter_end){
	 	Gtk::TreeModel::Row row = *Children_iter_cur;
		Glib::ustring hash_retrieved;
		row.get_value(0, hash_retrieved);

		std::vector<download_status>::iterator info_iter_cur, info_iter_end;
		info_iter_cur = info.begin();
		info_iter_end = info.end();
		bool entry_found = false;
		while(info_iter_cur != info_iter_end){
			if(hash_retrieved == info_iter_cur->hash){
				entry_found = true;
				break;
			}
			++info_iter_cur;
		}

		if(!entry_found){
			Children_iter_cur = download_list->erase(Children_iter_cur);
		}else{
			++Children_iter_cur;
		}
	}

	return true;
}

void gui::help_about()
{
	gui_about * GUI_About = new gui_about();
	Gtk::Main::run(*GUI_About);
	delete GUI_About;
}



void gui::settings_preferences()
{
	gui_window_preferences * prefs = new gui_window_preferences(&Client, &Server);
	Gtk::Main::run(*prefs);
	delete prefs;
}

bool gui::update_status_bar()
{
	std::string status; //holds entire status line

	//get the total client download speed
	int client_rate = Client.total_rate();
	std::stringstream client_rate_s;
	client_rate_s << convert::size_unit_select(client_rate) << "/s";

	//get the total server upload speed
	int server_rate = Server.total_rate();
	std::stringstream server_rate_s;
	server_rate_s << convert::size_unit_select(server_rate) << "/s";

	std::stringstream ss;
	ss << " D: " << client_rate_s.str();
	ss << std::string(16 - ss.str().size(), ' ');
	ss << " U: " << server_rate_s.str();
	ss << std::string(32 - ss.str().size(), ' ');
	ss << " P: " << Client.prime_count();
	ss << std::string(42 - ss.str().size(), ' ');
	ss << " QPS: " << database::connection::query_speed();

	if(Server.is_indexing()){
		ss << "  INDEXING";
	}

	statusbar->pop();
	statusbar->push(ss.str());

	return true;
}

void gui::upload_info_setup()
{
	//set up column
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> hash_t;
	Gtk::TreeModelColumn<Glib::ustring> IP_t;
	Gtk::TreeModelColumn<Glib::ustring> name_t;
	Gtk::TreeModelColumn<Glib::ustring> size_t;
	Gtk::TreeModelColumn<Glib::ustring> speed_t;
	Gtk::TreeModelColumn<int> percent_complete_t;

	column.add(hash_t);
	column.add(name_t);
	column.add(size_t);
	column.add(IP_t);
	column.add(speed_t);
	column.add(percent_complete_t);

	upload_list = Gtk::ListStore::create(column);
	upload_view->set_model(upload_list);

	//add columns
	upload_view->append_column(" Name ", name_t);
	upload_view->append_column(" Size ", size_t);
	upload_view->append_column(" IP ", IP_t);
	upload_view->append_column(" Speed ", speed_t);

	//display percentage progress bar
	Gtk::CellRendererProgress* cell = new Gtk::CellRendererProgress;
	int cols_count = upload_view->append_column(" Complete ", *cell);
	Gtk::TreeViewColumn* pColumn = upload_view->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), percent_complete_t);
}

bool gui::upload_info_refresh()
{
	//update upload info
	std::vector<upload_info> info;
	Server.current_uploads(info);

	std::vector<upload_info>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = info.begin();
	info_iter_end = info.end();
	while(info_iter_cur != info_iter_end){
		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> hash_t;
		Gtk::TreeModelColumn<Glib::ustring> name_t;
		Gtk::TreeModelColumn<Glib::ustring> size_t;
		Gtk::TreeModelColumn<Glib::ustring> IP_t;
		Gtk::TreeModelColumn<Glib::ustring> speed_t;
		Gtk::TreeModelColumn<int> percent_complete_t;

		column.add(hash_t);
		column.add(name_t);
		column.add(size_t);
		column.add(IP_t);
		column.add(speed_t);
		column.add(percent_complete_t);

		std::string speed = convert::size_unit_select(info_iter_cur->speed) + "/s";
		std::string size = convert::size_unit_select(info_iter_cur->size);

		//update rows
		bool entry_found = false;
		Gtk::TreeModel::Children children = upload_list->children();
		Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
		Children_iter_cur = children.begin();
		Children_iter_end = children.end();
		while(Children_iter_cur != Children_iter_end){
 			Gtk::TreeModel::Row row = *Children_iter_cur;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			if(hash_retrieved == info_iter_cur->hash){
				if(info_iter_cur->path.empty()){
					//path is empty which indicates this upload is a hash
					row[name_t] = info_iter_cur->hash;
				}else{
					row[name_t] = info_iter_cur->path.substr(info_iter_cur->path.find_last_of('/') + 1);
				}
				row[size_t] = size;
				row[IP_t] = info_iter_cur->IP;
				row[speed_t] = speed;
				row[percent_complete_t] = info_iter_cur->percent_complete;
				entry_found = true;
				break;
			}
			++Children_iter_cur;
		}

		if(!entry_found){
			Gtk::TreeModel::Row row = *(upload_list->append());
			row[hash_t] = info_iter_cur->hash;
			if(info_iter_cur->path.empty()){
				//path is empty which indicates this upload is a hash
				row[name_t] = info_iter_cur->hash;
			}else{
				row[name_t] = info_iter_cur->path.substr(info_iter_cur->path.find_last_of('/') + 1);
			}
			row[size_t] = size;
			row[IP_t] = info_iter_cur->IP;
			row[speed_t] = speed;
			row[percent_complete_t] = info_iter_cur->percent_complete;
		}
		++info_iter_cur;
	}

	//if no upload info exists remove all remaining rows
	if(info.size() == 0){
		upload_list->clear();
	}

	//remove rows without corresponding upload_info
	Gtk::TreeModel::Children children = upload_list->children();
	Gtk::TreeModel::Children::iterator Children_iter_cur, Children_iter_end;
	Children_iter_cur = children.begin();
	Children_iter_end = children.end();
	while(Children_iter_cur != Children_iter_end){
	 	Gtk::TreeModel::Row row = *Children_iter_cur;
		Glib::ustring hash_retrieved;
		row.get_value(0, hash_retrieved);

		std::vector<upload_info>::iterator info_iter_cur, info_iter_end;
		info_iter_cur = info.begin();
		info_iter_end = info.end();
		bool entry_found = false;
		while(info_iter_cur != info_iter_end){
			if(hash_retrieved == info_iter_cur->hash){
				entry_found = true;
				break;
			}
			++info_iter_cur;
		}

		if(!entry_found){
			Children_iter_cur = upload_list->erase(Children_iter_cur);
		}else{
			++Children_iter_cur;
		}
	}

	return true;
}