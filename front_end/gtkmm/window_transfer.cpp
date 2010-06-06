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
	int speed_col_num;
	if(type == download){
		speed_col_num = download_view->append_column(" Down ", column_speed) - 1;
	}else{
		speed_col_num = download_view->append_column(" Up ", column_speed) - 1;
	}
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

void window_transfer::info_tab_close(Gtk::ScrolledWindow * info_window)
{
	notebook->remove_page(*info_window);
}

bool window_transfer::refresh()
{
	std::list<p2p::transfer_info> TI = P2P.transfer();

	//add and update rows
	for(std::list<p2p::transfer_info>::iterator it_cur = TI.begin(),
		it_end = TI.end(); it_cur != it_end; ++it_cur)
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

	boost::optional<p2p::transfer_info> TI = P2P.transfer(hash);
	if(!TI){
		return;
	}
	std::string name = " " + convert::abbr(TI->name, 16) + " ";

	Gtk::HBox * HBox = Gtk::manage(new Gtk::HBox(false, 0));
	Gtk::Label * tab_label = Gtk::manage(new Gtk::Label(name));
	Gtk::Button * close_button = Gtk::manage(new Gtk::Button);
	Gtk::Image * close_image = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU));
	Gtk::ScrolledWindow * info_window = Gtk::manage(new window_transfer_info(P2P, *TI));

	//add/compose elements
	HBox->pack_start(*tab_label);
	close_button->add(*close_image);
	HBox->pack_start(*close_button);
	HBox->show_all_children();
	notebook->append_page(*info_window, *HBox, *tab_label);
	notebook->show_all_children();

	//signal to close tab, function pointer bound to window pointer associated with a tab
	close_button->signal_clicked().connect(sigc::bind<Gtk::ScrolledWindow *>(
		sigc::mem_fun(*this, &window_transfer::info_tab_close), info_window));
}
