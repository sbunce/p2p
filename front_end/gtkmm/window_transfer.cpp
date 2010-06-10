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
	transfer_view = Gtk::manage(new Gtk::TreeView);
	transfer_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);

	//options
	this->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC); //auto scroll bars
	transfer_view->set_headers_visible(true);
	transfer_view->set_rules_hint(true); //alternate row color

	//add/compose elements
	this->add(*transfer_view);
	column.add(name_column);
	column.add(size_column);
	column.add(speed_column);
	column.add(percent_complete_column);
	column.add(hash_column);
	column.add(update_column);

	//setup list to hold rows in treeview
	transfer_list = Gtk::ListStore::create(column);
	transfer_view->set_model(transfer_list);

	//add columns to transfer_view
	int name_col_num = transfer_view->append_column(" Name ", name_column) - 1;
	int size_col_num = transfer_view->append_column(" Size ", size_column) - 1;
	int speed_col_num;
	if(type == download){
		speed_col_num = transfer_view->append_column(" Down ", speed_column) - 1;
	}else{
		speed_col_num = transfer_view->append_column(" Up ", speed_column) - 1;
	}
	int complete_col_num = transfer_view->append_column(" Complete ", cell) - 1;
	transfer_view->get_column(complete_col_num)->add_attribute(cell.property_value(),
		percent_complete_column);

	//make columns sortable
	transfer_view->get_column(name_col_num)->set_sort_column(name_col_num);
	transfer_view->get_column(size_col_num)->set_sort_column(size_col_num);
	transfer_list->set_sort_func(size_col_num, sigc::bind(sigc::mem_fun(*this,
		&window_transfer::compare_SI), size_column));
	transfer_view->get_column(speed_col_num)->set_sort_column(speed_col_num);
	transfer_list->set_sort_func(speed_col_num, sigc::bind(sigc::mem_fun(*this,
		&window_transfer::compare_SI), speed_column));
	transfer_view->get_column(complete_col_num)->set_sort_column(complete_col_num);

	//right click menu
	popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
		Gtk::StockID(Gtk::Stock::INFO), sigc::mem_fun(*this, &window_transfer::transfer_info)));
	if(type == download){
		//menu that pops up when right click happens on download
		popup_menu.items().push_back(Gtk::Menu_Helpers::StockMenuElem(
			Gtk::StockID(Gtk::Stock::DELETE), sigc::mem_fun(*this,
			&window_transfer::download_delete)));
	}

	//signaled functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &window_transfer::refresh),
		settings::GUI_tick);
	transfer_view->signal_button_press_event().connect(sigc::mem_fun(*this,
		&window_transfer::click), false);
}

int window_transfer::compare_SI(const Gtk::TreeModel::iterator & lval,
	const Gtk::TreeModel::iterator & rval, const Gtk::TreeModelColumn<Glib::ustring> column)
{
	std::stringstream left_ss;
	left_ss << (*lval)[column];
	std::stringstream right_ss;
	right_ss << (*rval)[column];
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
		if(transfer_view->get_path_at_pos(event->x, event->y, path, column, x, y)){
			transfer_view->set_cursor(path);
		}else{
			transfer_view->get_selection()->unselect_all();
		}
	}else if(event->type == GDK_BUTTON_PRESS && event->button == 3){
		//right click
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn column_obj;
		Gtk::TreeViewColumn * column = &column_obj;
		if(transfer_view->get_path_at_pos(event->x, event->y, path, column, x, y)){
			popup_menu.popup(event->button, event->time);
			transfer_view->set_cursor(path);
		}else{
			transfer_view->get_selection()->unselect_all();
		}
	}
	return true;
}

void window_transfer::download_delete()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = transfer_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Glib::ustring hash = (*selected_row_iter)[hash_column];
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
		}else if(type == upload && it_cur->upload_hosts == 0){
			continue;
		}
		std::map<std::string, Gtk::TreeModel::Row>::iterator
			iter = Row_Idx.find(it_cur->hash);
		if(iter == Row_Idx.end()){
			//add
			Gtk::TreeModel::Row row = *(transfer_list->append());
			row[name_column] = it_cur->name;
			row[size_column] = convert::bytes_to_SI(it_cur->file_size);
			if(type == download){
				row[speed_column] = convert::bytes_to_SI(it_cur->download_speed) + "/s";
			}else{
				row[speed_column] = convert::bytes_to_SI(it_cur->upload_speed) + "/s";
			}
			row[percent_complete_column] = it_cur->percent_complete;
			row[hash_column] = it_cur->hash;
			row[update_column] = true;
			Row_Idx.insert(std::make_pair(it_cur->hash, row));
		}else{
			//update
			Gtk::TreeModel::Row row = iter->second;
			row[name_column] = it_cur->name;
			row[size_column] = convert::bytes_to_SI(it_cur->file_size);
			if(type == download){
				row[speed_column] = convert::bytes_to_SI(it_cur->download_speed) + "/s";
			}else{
				row[speed_column] = convert::bytes_to_SI(it_cur->upload_speed) + "/s";
			}
			row[percent_complete_column] = it_cur->percent_complete;
			row[update_column] = true;
		}
	}

	//remove rows not updated
	for(Gtk::TreeModel::Children::iterator it_cur = transfer_list->children().begin();
		it_cur != transfer_list->children().end();)
	{
		if((*it_cur)[update_column]){
			++it_cur;
		}else{
			Glib::ustring hash = (*it_cur)[hash_column];
			Row_Idx.erase(hash);
			it_cur = transfer_list->erase(it_cur);
		}
	}

	//make all rows as not updated for next call to refresh()
	for(Gtk::TreeModel::Children::iterator it_cur = transfer_list->children().begin(),
		it_end = transfer_list->children().end(); it_cur != it_end; ++it_cur)
	{
		(*it_cur)[update_column] = false;
	}

	return true;
}

void window_transfer::transfer_info()
{
	std::string hash;
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = transfer_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Glib::ustring tmp = (*selected_row_iter)[hash_column];
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
