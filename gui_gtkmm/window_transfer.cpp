#include "window_transfer.hpp"

window_transfer::window_transfer(
	p2p & P2P_in,
	const type Type_in
):
	P2P(P2P_in),
	Type(Type_in)
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
	column.add(column_peers);
	column.add(column_speed);
	column.add(column_percent_complete);
	download_list = Gtk::ListStore::create(column);
	download_view->set_model(download_list);

	download_view->append_column(" Name ", column_name);
	download_view->append_column(" Size ", column_size);
	download_view->append_column(" Peers ", column_peers);
	download_view->append_column(" Speed ", column_speed);

	//display percentage progress bar
	int cols_count = download_view->append_column(" Complete ", cell);
	Gtk::TreeViewColumn * pColumn = download_view->get_column(cols_count - 1);
	pColumn->add_attribute(cell.property_value(), column_percent_complete);

	//setup sorting on columns
	Gtk::TreeViewColumn * C;
	C = download_view->get_column(0); assert(C);
	C->set_sort_column(1);
	C = download_view->get_column(1); assert(C);
	C->set_sort_column(2);
	download_list->set_sort_func(2, sigc::mem_fun(*this, &window_transfer::compare_size));
	C = download_view->get_column(2); assert(C);
	C->set_sort_column(3);
	C = download_view->get_column(3); assert(C);
	C->set_sort_column(4);
	download_list->set_sort_func(4, sigc::mem_fun(*this, &window_transfer::compare_size));
	C = download_view->get_column(4); assert(C);
	C->set_sort_column(5);

	if(Type == download){
		//menu that pops up when right click happens on download
		downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
			Gtk::StockID(Gtk::Stock::MEDIA_PAUSE), sigc::mem_fun(*this,
			&window_transfer::pause)));
		downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
			Gtk::StockID(Gtk::Stock::DELETE), sigc::mem_fun(*this,
			&window_transfer::delete_download)));
	}

	//signaled functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &window_transfer::refresh),
		settings::GUI_TICK);
	download_view->signal_button_press_event().connect(sigc::mem_fun(*this,
		&window_transfer::click), false);
}

int window_transfer::compare_size(const Gtk::TreeModel::iterator & lval,
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

void window_transfer::delete_download()
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

bool window_transfer::click(GdkEventButton * event)
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
	}else if(Type == download && event->type == GDK_BUTTON_PRESS && event->button == 3){
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

bool window_transfer::refresh()
{
	std::vector<p2p::transfer> T;
	P2P.transfers(T);

	for(std::vector<p2p::transfer>::iterator T_iter_cur = T.begin(),
		T_iter_end = T.end(); T_iter_cur != T_iter_end; ++T_iter_cur)
	{
		//set up column
		std::stringstream ss;
		if(Type == download){
			ss << T_iter_cur->download_peers;
		}else{
			ss << T_iter_cur->upload_peers;
		}

		std::string speed;
		if(Type == download){
			speed = convert::size_SI(T_iter_cur->download_speed) + "/s";
		}else{
			speed = convert::size_SI(T_iter_cur->upload_speed) + "/s";
		}
		std::string size = convert::size_SI(T_iter_cur->file_size);

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
			if(hash_retrieved == T_iter_cur->hash){
				row[column_name] = T_iter_cur->name;
				row[column_size] = size;
				row[column_peers] = ss.str();
				row[column_speed] = speed;
				row[column_percent_complete] = T_iter_cur->percent_complete;
				entry_found = true;
				break;
			}
		}

		if(!entry_found){
			if(Type == download && (T_iter_cur->percent_complete == 100
				&& T_iter_cur->download_peers == 0))
			{
				continue;
			}
			if(Type == upload && T_iter_cur->upload_peers == 0){
				continue;
			}
			Gtk::TreeModel::Row row = *(download_list->append());
			row[column_hash] = T_iter_cur->hash;
			row[column_name] = T_iter_cur->name;
			row[column_size] = size;
			row[column_peers] = ss.str();
			row[column_speed] = speed;
			row[column_percent_complete] = T_iter_cur->percent_complete;
		}
	}

	//if no download info exists remove all remaining rows
	if(T.size() == 0){
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
		for(std::vector<p2p::transfer>::iterator T_iter_cur = T.begin(),
			T_iter_end = T.end(); T_iter_cur != T_iter_end; ++T_iter_cur)
		{
			if(hash_retrieved == T_iter_cur->hash){
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

void window_transfer::pause()
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
