#include "gui_vbox_search.hpp"

gui_vbox_search::gui_vbox_search(
	client & Client_in
):
	Client(&Client_in)
{
	vbox = this;

	//instantiation
	search_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	search_entry = Gtk::manage(new Gtk::Entry);
	search_button = Gtk::manage(new Gtk::Button(Gtk::Stock::FIND));
	search_view = Gtk::manage(new Gtk::TreeView);
	search_HBox = Gtk::manage(new Gtk::HBox(false, 0));

	//options and ownership
	search_entry->set_max_length(255);
	search_HBox->pack_start(*search_entry);
	search_HBox->pack_start(*search_button, Gtk::PACK_SHRINK, 5);
	search_view->set_headers_visible(true);
	search_view->set_rules_hint(true); //enable alternating row background color
	vbox->pack_start(*search_HBox, Gtk::PACK_SHRINK, 0);
	search_scrolled_window->add(*search_view);
	vbox->pack_start(*search_scrolled_window);

	//setup colums for treeview
	column.add(column_hash);
	column.add(column_name);
	column.add(column_size);
	column.add(column_IP);
	search_list = Gtk::ListStore::create(column);
	search_view->set_model(search_list);
	search_view->append_column("  Name  ", column_name);
	//0 is the visible column, 1 is the actual column
	search_view->get_column(0)->set_sort_column(1);
	search_view->append_column("  Size  ", column_size);
	search_view->get_column(1)->set_sort_column(2);
	search_list->set_sort_func(2, sigc::mem_fun(*this, &gui_vbox_search::compare_file_size));
	search_view->append_column("  IP  ", column_IP);

	//menu that pops up when right click happens
	Gtk::Menu::MenuList & menu_list = search_popup_menu.items();
	menu_list.push_back(Gtk::Menu_Helpers::MenuElem("_Download", sigc::mem_fun(*this, &gui_vbox_search::download_file)));

	//signaled functions
	search_view->signal_button_press_event().connect(sigc::mem_fun(*this, &gui_vbox_search::search_click), false);
	search_entry->signal_activate().connect(sigc::mem_fun(*this, &gui_vbox_search::search_input), false);
	search_button->signal_clicked().connect(sigc::mem_fun(*this, &gui_vbox_search::search_input), false);
}

int gui_vbox_search::compare_file_size(const Gtk::TreeModel::iterator & lval, const Gtk::TreeModel::iterator & rval)
{
	Gtk::TreeModel::Row row_lval = *lval;
	Gtk::TreeModel::Row row_rval = *rval;

	//convert to std::string, involves a lot of copying but no known alternative
	std::stringstream ss;
	ss << row_lval[column_size];
	std::string left = ss.str();
	ss.str(""); ss.clear();
	ss << row_rval[column_size];
	std::string right = ss.str();

	return convert::size_SI_cmp(left, right);
}

void gui_vbox_search::download_file()
{
	Glib::RefPtr<Gtk::TreeView::Selection> refSelection = search_view->get_selection();
	if(refSelection){
		Gtk::TreeModel::iterator selected_row_iter = refSelection->get_selected();
		if(selected_row_iter){
			Gtk::TreeModel::Row row = *selected_row_iter;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);

			std::vector<download_info>::iterator iter_cur, iter_end;
			iter_cur = Search_Info.begin();
			iter_end = Search_Info.end();
			while(iter_cur != iter_end){
				if(iter_cur->hash == hash_retrieved){
					Client->start_download(*iter_cur);
					break;
				}
				++iter_cur;
			}
		}
	}
}

bool gui_vbox_search::search_click(GdkEventButton * event)
{
	if(event->type == GDK_BUTTON_PRESS && event->button == 3){
		search_popup_menu.popup(event->button, event->time);

		//select the row when the user right clicks on it
		int x, y;
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn columnObject;
		Gtk::TreeViewColumn * column = &columnObject;
		if(search_view->get_path_at_pos((int)event->x, (int)event->y, path, column, x, y)){
			search_view->set_cursor(path);
		}
		return true;
	}
	return false;
}

void gui_vbox_search::search_input()
{
	std::string input_text = search_entry->get_text();
	Client->search(input_text, Search_Info);
	search_info_refresh();
}

void gui_vbox_search::search_info_refresh()
{
	//clear all previous results
	search_list->clear();

	std::vector<download_info>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = Search_Info.begin();
	info_iter_end = Search_Info.end();
	while(info_iter_cur != info_iter_end){

		std::vector<std::string>::iterator IP_iter_cur, IP_iter_end;
		IP_iter_cur = info_iter_cur->IP.begin();
		IP_iter_end = info_iter_cur->IP.end();
		std::string IP;
		while(IP_iter_cur != IP_iter_end){
			IP += *IP_iter_cur + ";";
			++IP_iter_cur;
		}

		std::string size = convert::size_SI(info_iter_cur->size);

		//add row
		Gtk::TreeModel::Row row = *(search_list->append());
		row[column_hash] = info_iter_cur->hash;
		row[column_name] = info_iter_cur->name;
		row[column_size] = size;
		row[column_IP] = IP;

		++info_iter_cur;
	}
}
