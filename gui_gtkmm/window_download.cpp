#include "window_download.hpp"

window_download::window_download(
	p2p & P2P_in,
	Gtk::Notebook * notebook_in
):
	P2P(P2P_in),
	notebook(notebook_in)
{
	window = this;

	//instantiation
	download_view = Gtk::manage(new Gtk::TreeView);
	download_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);

	//options and ownership
	download_view->set_headers_visible(true);
	download_view->set_rules_hint(true); //alternate row background colors
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

	//setup sorting on columns
	Gtk::TreeViewColumn * C;
	C = download_view->get_column(0); assert(C);
	C->set_sort_column(1);
	C = download_view->get_column(1); assert(C);
	C->set_sort_column(2);
	download_list->set_sort_func(2, sigc::mem_fun(*this, &window_download::compare_size));
	C = download_view->get_column(2); assert(C);
	C->set_sort_column(3);
	C = download_view->get_column(3); assert(C);
	C->set_sort_column(4);
	download_list->set_sort_func(4, sigc::mem_fun(*this, &window_download::compare_size));
	C = download_view->get_column(4); assert(C);
	C->set_sort_column(5);

	//menu that pops up when right click happens on download
	downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
		Gtk::StockID(Gtk::Stock::MEDIA_PAUSE), sigc::mem_fun(*this,
		&window_download::pause_download)));
	downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
		Gtk::StockID(Gtk::Stock::DELETE), sigc::mem_fun(*this,
		&window_download::delete_download)));

	//signaled functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&window_download::download_info_refresh), settings::GUI_TICK);
	download_view->signal_button_press_event().connect(sigc::mem_fun(*this,
		&window_download::download_click), false);
}

int window_download::compare_size(const Gtk::TreeModel::iterator & lval,
	const Gtk::TreeModel::iterator & rval)
{
	Gtk::TreeModel::Row row_lval = *lval;
	Gtk::TreeModel::Row row_rval = *rval;

	std::stringstream ss;
	ss << row_lval[column_size];
	std::string left = ss.str();
	ss.str(""); ss.clear();
	ss << row_rval[column_size];
	std::string right = ss.str();

	return convert::size_SI_cmp(left, right);
}

void window_download::delete_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Gtk::TreeModel::Row row = *selected_row_iter;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			P2P.remove_download(hash_retrieved);
		}
	}
}

bool window_download::download_click(GdkEventButton * event)
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

bool window_download::download_info_refresh()
{
	//update download info
	std::vector<download_status> status;
	P2P.current_downloads(status);

	for(std::vector<download_status>::iterator info_iter_cur = status.begin(),
		info_iter_end = status.end(); info_iter_cur != info_iter_end; ++info_iter_cur)
	{
		//set up column
		std::stringstream ss;
		ss << info_iter_cur->servers.size();
		std::string speed = convert::size_SI(info_iter_cur->total_speed) + "/s";
		std::string size = convert::size_SI(info_iter_cur->size);

		//update rows
		bool entry_found = false;
		Gtk::TreeModel::Children children = download_list->children();
		for(Gtk::TreeModel::Children::iterator child_iter_cur = children.begin(),
			child_iter_end = children.end(); child_iter_cur != child_iter_end;
			++child_iter_cur)
		{
 			Gtk::TreeModel::Row row = *child_iter_cur;
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
	}

	//if no download info exists remove all remaining rows
	if(status.size() == 0){
		download_list->clear();
	}

	//remove rows without corresponding download_info
	Gtk::TreeModel::Children children = download_list->children();
	Gtk::TreeModel::Children::iterator child_iter_cur = children.begin(),
		child_iter_end = children.end();
	while(child_iter_cur != child_iter_end){
	 	Gtk::TreeModel::Row row = *child_iter_cur;
		Glib::ustring hash_retrieved;
		row.get_value(0, hash_retrieved);

		bool entry_found = false;
		for(std::vector<download_status>::iterator info_iter_cur = status.begin(),
			info_iter_end = status.end(); info_iter_cur != info_iter_end; ++info_iter_cur)
		{
			if(hash_retrieved == info_iter_cur->hash){
				entry_found = true;
				break;
			}
		}

		if(!entry_found){
			child_iter_cur = download_list->erase(child_iter_cur);
		}else{
			++child_iter_cur;
		}
	}
	return true;
}

void window_download::pause_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Gtk::TreeModel::Row row = *selected_row_iter;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			P2P.pause_download(hash_retrieved);
		}
	}
}
