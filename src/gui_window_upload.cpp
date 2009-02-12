#include "gui_window_upload.hpp"

gui_window_upload::gui_window_upload(
	server & Server_in
):
	Server(&Server_in)
{
	window = this;

	//instantiation
	upload_view = Gtk::manage(new Gtk::TreeView);

	//options and ownership
	upload_view->set_headers_visible(true);
	upload_view->set_rules_hint(true);
	window->add(*upload_view);

	column.add(column_hash);
	column.add(column_name);
	column.add(column_size);
	column.add(column_IP);
	column.add(column_speed);
	column.add(column_percent_complete);
	upload_list = Gtk::ListStore::create(column);
	upload_view->set_model(upload_list);
	upload_view->append_column(" Name ", column_name);
	upload_view->append_column(" Size ", column_size);
	upload_view->append_column(" IP ", column_IP);
	upload_view->append_column(" Speed ", column_speed);

	//percentage progress bar for percent_complete column
	cell = Gtk::manage(new Gtk::CellRendererProgress);
	int cols_count = upload_view->append_column(" Complete ", *cell);
	Gtk::TreeViewColumn * pColumn = upload_view->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_value(), column_percent_complete);

	//timed functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui_window_upload::upload_info_refresh), global::GUI_TICK);
}

bool gui_window_upload::upload_info_refresh()
{
	//update upload info
	std::vector<upload_info> info;
	Server->current_uploads(info);

	std::vector<upload_info>::iterator info_iter_cur, info_iter_end;
	info_iter_cur = info.begin();
	info_iter_end = info.end();
	while(info_iter_cur != info_iter_end){
		std::string speed = convert::size_SI(info_iter_cur->speed) + "/s";
		std::string size = convert::size_SI(info_iter_cur->size);

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
					row[column_name] = info_iter_cur->hash;
				}else{
					row[column_name] = info_iter_cur->path.substr(info_iter_cur->path.find_last_of('/') + 1);
				}
				row[column_size] = size;
				row[column_IP] = info_iter_cur->IP;
				row[column_speed] = speed;
				row[column_percent_complete] = info_iter_cur->percent_complete;
				entry_found = true;
				break;
			}
			++Children_iter_cur;
		}

		if(!entry_found){
			Gtk::TreeModel::Row row = *(upload_list->append());
			row[column_hash] = info_iter_cur->hash;
			if(info_iter_cur->path.empty()){
				//path is empty which indicates this upload is a hash
				row[column_name] = info_iter_cur->hash;
			}else{
				row[column_name] = info_iter_cur->path.substr(info_iter_cur->path.find_last_of('/') + 1);
			}
			row[column_size] = size;
			row[column_IP] = info_iter_cur->IP;
			row[column_speed] = speed;
			row[column_percent_complete] = info_iter_cur->percent_complete;
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
