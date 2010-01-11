#include "window_transfer.hpp"

window_transfer::window_transfer(
	p2p & P2P_in,
	const type Type_in
):
	P2P(P2P_in),
	Type(Type_in)
{
	window = this;

	//instantiate
	download_view = Gtk::manage(new Gtk::TreeView);
	window->add(*download_view);
	download_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);

	//options
	download_view->set_headers_visible(true);
	download_view->set_rules_hint(true); //alternate row color

	//set up column
	column.add(column_name);
	column.add(column_size);
	column.add(column_peers);
	column.add(column_speed);
	column.add(column_percent_complete);
	column.add(column_hash); hash_col_num = 5;
	column.add(column_update); update_col_num = 6;

	//setup list to hold rows in treeview
	download_list = Gtk::ListStore::create(column);
	download_view->set_model(download_list);

	//add columns to download_view
	int name_col_num = download_view->append_column(" Name ", column_name) - 1;
	int size_col_num = download_view->append_column(" Size ", column_size) - 1;
	int peers_col_num = download_view->append_column(" Peers ", column_peers) - 1;
	int speed_col_num = download_view->append_column(" Speed ", column_speed) - 1;
	int complete_col_num = download_view->append_column(" Complete ", cell) - 1;
	download_view->get_column(complete_col_num)->add_attribute(cell.property_value(),
		column_percent_complete);

	//make columns sortable
	download_view->get_column(name_col_num)->set_sort_column(name_col_num);
	download_view->get_column(size_col_num)->set_sort_column(size_col_num);
	download_list->set_sort_func(size_col_num, sigc::mem_fun(*this,
		&window_transfer::compare_size));
	download_view->get_column(peers_col_num)->set_sort_column(peers_col_num);
	download_view->get_column(speed_col_num)->set_sort_column(speed_col_num);
	download_list->set_sort_func(speed_col_num, sigc::mem_fun(*this,
		&window_transfer::compare_size));
	download_view->get_column(complete_col_num)->set_sort_column(complete_col_num);

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
	std::stringstream left_ss;
	left_ss << (*lval)[column_size];
	std::stringstream right_ss;
	right_ss << (*rval)[column_size];
	return convert::size_SI_cmp(left_ss.str(), right_ss.str());
}

void window_transfer::delete_download()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Glib::ustring hash = (*selected_row_iter)[column_hash];
			P2P.remove_download(hash);
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
		if(download_view->get_path_at_pos((int)event->x, (int)event->y, path,
			column, x, y))
		{
			download_view->set_cursor(path);
		}else{
			download_view->get_selection()->unselect_all();
		}
	}else if(Type == download && event->type == GDK_BUTTON_PRESS
		&& event->button == 3)
	{
		//right click
		downloads_popup_menu.popup(event->button, event->time);
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(download_view->get_path_at_pos((int)event->x, (int)event->y, path,
			column, x, y))
		{
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

	//add and update rows
	for(std::vector<p2p::transfer>::iterator iter_cur = T.begin(),
		iter_end = T.end(); iter_cur != iter_end; ++iter_cur)
	{
		std::map<std::string, Gtk::TreeModel::Row>::iterator
			iter = Row_Index.find(iter_cur->hash);
		if(iter == Row_Index.end()){
			//add
			if(Type == download && iter_cur->download_peers == 0){
				continue;
			}
			if(Type == upload && iter_cur->upload_peers == 0){
				continue;
			}
			Gtk::TreeModel::Row row = *(download_list->append());
			row[column_name] = iter_cur->name;
			row[column_size] = convert::size_SI(iter_cur->file_size);
			std::stringstream peers;
			if(Type == download){
				peers << iter_cur->download_peers;
			}else{
				peers << iter_cur->upload_peers;
			}
			row[column_peers] = peers.str();
			if(Type == download){
				row[column_speed] = convert::size_SI(iter_cur->download_speed) + "/s";
			}else{
				row[column_speed] = convert::size_SI(iter_cur->upload_speed) + "/s";
			}
			row[column_percent_complete] = iter_cur->percent_complete;
			row[column_hash] = iter_cur->hash;
			row[column_update] = true;
			Row_Index.insert(std::make_pair(iter_cur->hash, row));
		}else{
			//update
			Gtk::TreeModel::Row row = iter->second;
			row[column_name] = iter_cur->name;
			row[column_size] = convert::size_SI(iter_cur->file_size);
			std::stringstream peers;
			if(Type == download){
				peers << iter_cur->download_peers;
			}else{
				peers << iter_cur->upload_peers;
			}
			row[column_peers] = peers.str();
			if(Type == download){
				row[column_speed] = convert::size_SI(iter_cur->download_speed) + "/s";
			}else{
				row[column_speed] = convert::size_SI(iter_cur->upload_speed) + "/s";
			}
			row[column_percent_complete] = iter_cur->percent_complete;
			row[column_update] = true;
		}
	}

	//remove rows not updated
	for(Gtk::TreeModel::Children::iterator iter_cur = download_list->children().begin();
		iter_cur != download_list->children().end();)
	{
		if((*iter_cur)[column_update]){
			++iter_cur;
		}else{
			Glib::ustring hash = (*iter_cur)[column_hash];
			Row_Index.erase(hash);
			iter_cur = download_list->erase(iter_cur);
		}
	}

	//make all rows as not updated for next call to refresh()
	for(Gtk::TreeModel::Children::iterator iter_cur = download_list->children().begin(),
		iter_end = download_list->children().end(); iter_cur != iter_end; ++iter_cur)
	{
		(*iter_cur)[column_update] = false;
	}

	return true;
}

void window_transfer::pause()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Glib::ustring hash = (*selected_row_iter)[column_hash];
			P2P.pause_download(hash);
		}
	}
}
