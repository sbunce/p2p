#include "window_upload.hpp"

window_upload::window_upload(
	p2p & P2P_in
):
	P2P(P2P_in)
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

	//setup sorting on columns
	Gtk::TreeViewColumn * C;
	C = upload_view->get_column(0); assert(C);
	C->set_sort_column(1);
	C = upload_view->get_column(1); assert(C);
	C->set_sort_column(2);
	upload_list->set_sort_func(2, sigc::mem_fun(*this, &window_upload::compare_size));
	C = upload_view->get_column(2); assert(C);
	C->set_sort_column(3);
	C = upload_view->get_column(3); assert(C);
	C->set_sort_column(4);
	upload_list->set_sort_func(4, sigc::mem_fun(*this, &window_upload::compare_size));
	C = upload_view->get_column(4); assert(C);
	C->set_sort_column(5);

	//timed functions
	Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&window_upload::upload_info_refresh), settings::GUI_TICK);
}

int window_upload::compare_size(const Gtk::TreeModel::iterator & lval,
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

bool window_upload::upload_info_refresh()
{
	std::vector<upload_status> status;
	P2P.uploads(status);

	for(std::vector<upload_status>::iterator status_iter_cur = status.begin(),
		status_iter_end = status.end(); status_iter_cur != status_iter_end;
		++status_iter_cur)
	{
		std::string speed = convert::size_SI(status_iter_cur->speed) + "/s";
		std::string size = convert::size_SI(status_iter_cur->size);

		//update rows
		bool entry_found = false;
		Gtk::TreeModel::Children children = upload_list->children();
		for(Gtk::TreeModel::Children::iterator child_iter_cur = children.begin(),
			child_iter_end = children.end(); child_iter_cur != child_iter_end; ++child_iter_cur)
		{
 			Gtk::TreeModel::Row row = *child_iter_cur;
			Glib::ustring hash_retrieved;
			row.get_value(0, hash_retrieved);
			if(hash_retrieved == status_iter_cur->hash){
				if(status_iter_cur->path.empty()){
					//path is empty which indicates this upload is a hash
					row[column_name] = status_iter_cur->hash;
				}else{
					row[column_name] = status_iter_cur->path.substr(status_iter_cur->path.find_last_of('/') + 1);
				}
				row[column_size] = size;
				row[column_IP] = status_iter_cur->IP;
				row[column_speed] = speed;
				row[column_percent_complete] = status_iter_cur->percent_complete;
				entry_found = true;
				break;
			}
		}

		if(!entry_found){
			Gtk::TreeModel::Row row = *(upload_list->append());
			row[column_hash] = status_iter_cur->hash;
			if(status_iter_cur->path.empty()){
				//path is empty which indicates this upload is a hash
				row[column_name] = status_iter_cur->hash;
			}else{
				row[column_name] = status_iter_cur->path.substr(status_iter_cur->path.find_last_of('/') + 1);
			}
			row[column_size] = size;
			row[column_IP] = status_iter_cur->IP;
			row[column_speed] = speed;
			row[column_percent_complete] = status_iter_cur->percent_complete;
		}
	}

	//if no upload info exists remove all remaining rows
	if(status.size() == 0){
		upload_list->clear();
	}

	//remove rows without corresponding upload_info
	Gtk::TreeModel::Children children = upload_list->children();
	Gtk::TreeModel::Children::iterator Children_iter_cur = children.begin(),
		Children_iter_end = children.end();
	while(Children_iter_cur != Children_iter_end){
	 	Gtk::TreeModel::Row row = *Children_iter_cur;
		Glib::ustring hash_retrieved;
		row.get_value(0, hash_retrieved);

		bool entry_found = false;
		for(std::vector<upload_status>::iterator status_iter_cur = status.begin(),
			status_iter_end = status.end(); status_iter_cur != status_iter_end; ++status_iter_cur)
		{
			if(hash_retrieved == status_iter_cur->hash){
				entry_found = true;
				break;
			}
		}

		if(!entry_found){
			Children_iter_cur = upload_list->erase(Children_iter_cur);
		}else{
			++Children_iter_cur;
		}
	}

	return true;
}
