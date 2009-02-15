#include "gui_window_download.hpp"

gui_window_download::gui_window_download(
	p2p & P2P_in,
	Gtk::Notebook * notebook_in
):
	P2P(&P2P_in),
	notebook(notebook_in)
{
	window = this;

	//instantiation
	download_view = Gtk::manage(new Gtk::TreeView);
	download_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);

	//options and ownership
	download_view->set_headers_visible(true);
	download_view->set_rules_hint(true); //alternating row background colors
	window->add(*download_view);

	//set up column
	column.add(column_hash);
	column.add(column_name);
	column.add(column_size);
	column.add(column_servers);
	column.add(column_speed);
	column.add(column_percent_complete);
	download_list = Gtk::ListStore::create(column);
	download_view->set_model(download_list);
	download_view->append_column(" Name ", column_name);
	download_view->append_column(" Size ", column_size);
	download_view->append_column(" Peers ", column_servers);
	download_view->append_column(" Speed ", column_speed);

	//display percentage progress bar
	cell = Gtk::manage(new Gtk::CellRendererProgress);
	int cols_count = download_view->append_column(" Complete ", *cell);
	Gtk::TreeViewColumn * pColumn = download_view->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), column_percent_complete);

	//menu that pops up when right click happens on download
	downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::CANCEL), sigc::mem_fun(*this, &gui_window_download::cancel_download)));
	downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::INFO), sigc::mem_fun(*this, &gui_window_download::download_info_tab)));

	//signaled functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui_window_download::download_info_refresh), global::GUI_TICK);
	download_view->signal_button_press_event().connect(sigc::mem_fun(*this, &gui_window_download::download_click), false);
}

void gui_window_download::cancel_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator iter0 = refSelection->get_selected();
		if(iter0){
			Gtk::TreeModel::Row row = *iter0;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			P2P->stop_download(hash_retrieved);
		}
	}
}

bool gui_window_download::download_click(GdkEventButton * event)
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

void gui_window_download::download_info_tab()
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
	std::string path;
	boost::uint64_t tree_size, file_size;
	if(!P2P->file_info(root_hash, path, tree_size, file_size)){
		LOGGER << "clicked download doesn't exist";
		return;
	}

	std::set<std::string>::iterator iter = open_info_tabs.find(root_hash);
	if(iter != open_info_tabs.end()){
		LOGGER << "tab for " << root_hash << " already open";
		return;
	}else{
		open_info_tabs.insert(root_hash);
	}

	Gtk::HBox * hbox;           //separates tab label from button
	Gtk::Label * tab_label;     //tab text
	Gtk::Button * close_button; //clickable close button

	gui_window_download_status * status_window = Gtk::manage(new gui_window_download_status(
		root_hash,
		path,
		tree_size,
		file_size,
		hbox,         //set in ctor
		tab_label,    //set in ctor
		close_button, //set in ctor
		*P2P
	));

	notebook->append_page(*status_window, *hbox, *tab_label);
	notebook->show_all_children();

	//set tab window to refresh
	sigc::connection tab_conn = Glib::signal_timeout().connect(
		sigc::mem_fun(status_window, &gui_window_download_status::refresh),
		global::GUI_TICK
	);

	//signal to close tab, function pointer bound to window pointer associated with a tab
	close_button->signal_clicked().connect(
		sigc::bind<gui_window_download_status *, std::string, sigc::connection>(
			sigc::mem_fun(*this, &gui_window_download::download_info_tab_close),
			status_window, root_hash, tab_conn
		)
	);
}

void gui_window_download::download_info_tab_close(gui_window_download_status * status_window, std::string root_hash, sigc::connection tab_conn)
{
	tab_conn.disconnect();                 //stop window from refreshing
	notebook->remove_page(*status_window); //remove from notebook
	open_info_tabs.erase(root_hash);
}

bool gui_window_download::download_info_refresh()
{
	//update download info
	std::vector<download_status> info;
	P2P->current_downloads(info);

	std::vector<download_status>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = info.begin();
	info_iter_end = info.end();
	while(info_iter_cur != info_iter_end){
		//set up column
		std::stringstream ss;
		ss << info_iter_cur->IP.size();
		std::string speed = convert::size_SI(info_iter_cur->total_speed) + "/s";
		std::string size = convert::size_SI(info_iter_cur->size);

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
				row[column_name] = info_iter_cur->name;
				row[column_size] = size;
				row[column_servers] = ss.str();
				row[column_speed] = speed;
				row[column_percent_complete] = info_iter_cur->percent_complete;
				entry_found = true;
				break;
			}
			++Children_iter_cur;
		}

		if(!entry_found){
			Gtk::TreeModel::Row row = *(download_list->append());
			row[column_hash] = info_iter_cur->hash;
			row[column_name] = info_iter_cur->name;
			row[column_size] = size;
			row[column_servers] = ss.str();
			row[column_speed] = speed;
			row[column_percent_complete] = info_iter_cur->percent_complete;
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
