#include "window_transfer.hpp"

window_transfer::window_transfer(
	const type_t type_in,
	p2p & P2P_in,
	Gtk::Notebook * notebook_in
):
	type(type_in),
	P2P(P2P_in),
	notebook(notebook_in)
{
	download_view = Gtk::manage(new Gtk::TreeView);
	download_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);

	//options
	this->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC); //auto scroll bars
	download_view->set_headers_visible(true);
	download_view->set_rules_hint(true); //alternate row color

	//add/compose elements
	this->add(*download_view);
	column.add(column_name);
	column.add(column_size);
	column.add(column_speed);
	column.add(column_percent_complete);
	column.add(column_hash);
	column.add(column_update);

	//setup list to hold rows in treeview
	download_list = Gtk::ListStore::create(column);
	download_view->set_model(download_list);

	//add columns to download_view
	int name_col_num = download_view->append_column(" Name ", column_name) - 1;
	int size_col_num = download_view->append_column(" Size ", column_size) - 1;
	int speed_col_num = download_view->append_column(" Speed ", column_speed) - 1;
	int complete_col_num = download_view->append_column(" Complete ", cell) - 1;
	download_view->get_column(complete_col_num)->add_attribute(cell.property_value(),
		column_percent_complete);

	//make columns sortable
	download_view->get_column(name_col_num)->set_sort_column(name_col_num);
	download_view->get_column(size_col_num)->set_sort_column(size_col_num);
	download_list->set_sort_func(size_col_num, sigc::mem_fun(*this,
		&window_transfer::compare_size));
	download_view->get_column(speed_col_num)->set_sort_column(speed_col_num);
	download_list->set_sort_func(speed_col_num, sigc::mem_fun(*this,
		&window_transfer::compare_size));
	download_view->get_column(complete_col_num)->set_sort_column(complete_col_num);

	//right click menu
	downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
		Gtk::StockID(Gtk::Stock::INFO), sigc::mem_fun(*this, &window_transfer::transfer_info)));
	if(type == download){
		//menu that pops up when right click happens on download
		downloads_popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
			Gtk::StockID(Gtk::Stock::DELETE), sigc::mem_fun(*this,
			&window_transfer::download_delete)));
	}

	//signaled functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &window_transfer::refresh),
		settings::GUI_tick);
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
	return convert::SI_cmp(left_ss.str(), right_ss.str());
}

bool window_transfer::click(GdkEventButton * event)
{
	if(event->type == GDK_BUTTON_PRESS && event->button == 1){
		//left click
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn column_obj;
		Gtk::TreeViewColumn * column = &column_obj;
		if(download_view->get_path_at_pos(event->x, event->y, path, column, x, y)){
			download_view->set_cursor(path);
		}else{
			download_view->get_selection()->unselect_all();
		}
	}else if(type == download && event->type == GDK_BUTTON_PRESS
		&& event->button == 3)
	{
		//right click
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn column_obj;
		Gtk::TreeViewColumn * column = &column_obj;
		if(download_view->get_path_at_pos(event->x, event->y, path, column, x, y)){
			downloads_popup_menu.popup(event->button, event->time);
			download_view->set_cursor(path);
		}else{
			download_view->get_selection()->unselect_all();
		}
	}
	return true;
}

void window_transfer::download_delete()
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

void window_transfer::transfer_info()
{
	std::string hash;
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = download_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Glib::ustring tmp = (*selected_row_iter)[column_hash];
			hash = tmp;
		}
	}
}

/*
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
		settings::GUI_TICK
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
*/

bool window_transfer::refresh()
{
	std::list<p2p::transfer_info> T = P2P.transfers();

	//add and update rows
	for(std::list<p2p::transfer_info>::iterator it_cur = T.begin(),
		it_end = T.end(); it_cur != it_end; ++it_cur)
	{
		if(type == download && it_cur->percent_complete == 100)
		{
			continue;
		}else if(type == upload && it_cur->upload_peers == 0){
			continue;
		}
		std::map<std::string, Gtk::TreeModel::Row>::iterator
			iter = Row_Idx.find(it_cur->hash);
		if(iter == Row_Idx.end()){
			//add
			Gtk::TreeModel::Row row = *(download_list->append());
			row[column_name] = it_cur->name;
			row[column_size] = convert::bytes_to_SI(it_cur->file_size);
			if(type == download){
				row[column_speed] = convert::bytes_to_SI(it_cur->download_speed) + "/s";
			}else{
				row[column_speed] = convert::bytes_to_SI(it_cur->upload_speed) + "/s";
			}
			row[column_percent_complete] = it_cur->percent_complete;
			row[column_hash] = it_cur->hash;
			row[column_update] = true;
			Row_Idx.insert(std::make_pair(it_cur->hash, row));
		}else{
			//update
			Gtk::TreeModel::Row row = iter->second;
			row[column_name] = it_cur->name;
			row[column_size] = convert::bytes_to_SI(it_cur->file_size);
			if(type == download){
				row[column_speed] = convert::bytes_to_SI(it_cur->download_speed) + "/s";
			}else{
				row[column_speed] = convert::bytes_to_SI(it_cur->upload_speed) + "/s";
			}
			row[column_percent_complete] = it_cur->percent_complete;
			row[column_update] = true;
		}
	}

	//remove rows not updated
	for(Gtk::TreeModel::Children::iterator it_cur = download_list->children().begin();
		it_cur != download_list->children().end();)
	{
		if((*it_cur)[column_update]){
			++it_cur;
		}else{
			Glib::ustring hash = (*it_cur)[column_hash];
			Row_Idx.erase(hash);
			it_cur = download_list->erase(it_cur);
		}
	}

	//make all rows as not updated for next call to refresh()
	for(Gtk::TreeModel::Children::iterator it_cur = download_list->children().begin(),
		it_end = download_list->children().end(); it_cur != it_end; ++it_cur)
	{
		(*it_cur)[column_update] = false;
	}

	return true;
}
