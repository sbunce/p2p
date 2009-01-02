#include "gui_download_status.h"

gui_download_status::gui_download_status(
	const std::string root_hash_in,
	Gtk::Label * tab_label,
	client * Client_in
):
	root_hash(root_hash_in),
	tree_size_bytes(0),
	file_size_bytes(0)
{
	Client = Client_in;

	std::string name;             //starts as path
	std::string untruncated_name; //full name for display within tab
	if(Client->file_info(root_hash, name, tree_size_bytes, file_size_bytes)){
		std::string label_name = name;
		if(!label_name.empty()){
			//isolate file name
			label_name = label_name.substr(label_name.find_last_of('/') + 1);
		}
		untruncated_name = label_name;
		int max_tab_label_size = 24;
		if(label_name.size() > max_tab_label_size){
			label_name = label_name.substr(0, max_tab_label_size);
			label_name += "..";
		}
		tab_label->set_text(" " + label_name + " ");
	}else{
		tab_label->set_text("error: no download");
	}

	std::string tree_size_str = convert::size_unit_select(tree_size_bytes);
	std::string file_size_str = convert::size_unit_select(file_size_bytes);

	window = this;
	window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);

	vbox = Gtk::manage(new Gtk::VBox(false, 0));
	info_fixed = Gtk::manage(new Gtk::Fixed);
	servers_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	servers_view = Gtk::manage(new Gtk::TreeView);
	root_hash_label = Gtk::manage(new Gtk::Label("Root Hash: "));
	root_hash_value = Gtk::manage(new Gtk::Label(root_hash_in));
	hash_tree_size_label = Gtk::manage(new Gtk::Label("Tree Size: "));
	hash_tree_size_value = Gtk::manage(new Gtk::Label(tree_size_str));
	hash_tree_percent_label = Gtk::manage(new Gtk::Label("Complete: "));
	hash_tree_percent_value = Gtk::manage(new Gtk::Label("0%"));
	file_name_label = Gtk::manage(new Gtk::Label("File Name: "));
	file_name_value = Gtk::manage(new Gtk::Label(untruncated_name));
	file_size_label = Gtk::manage(new Gtk::Label("File Size: "));
	file_size_value = Gtk::manage(new Gtk::Label(file_size_str));
	file_percent_label = Gtk::manage(new Gtk::Label("Complete: "));
	file_percent_value = Gtk::manage(new Gtk::Label("0%"));
	total_speed_label = Gtk::manage(new Gtk::Label("Speed: "));
	total_speed_value = Gtk::manage(new Gtk::Label);
	servers_connected_label = Gtk::manage(new Gtk::Label("Servers: "));
	servers_connected_value = Gtk::manage(new Gtk::Label);

	add(*vbox);
	vbox->pack_start(*info_fixed, Gtk::PACK_SHRINK, 8);
	vbox->pack_start(*servers_scrolled_window);

	servers_scrolled_window->add(*servers_view);
	servers_scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);

	info_fixed->put(*root_hash_label, 8, 0);
	info_fixed->put(*root_hash_value, 85, 0);
	info_fixed->put(*hash_tree_size_label, 8, 16);
	info_fixed->put(*hash_tree_size_value, 85, 16);
	info_fixed->put(*hash_tree_percent_label, 8, 32);
	info_fixed->put(*hash_tree_percent_value, 85, 32);
	info_fixed->put(*file_name_label, 8, 64);
	info_fixed->put(*file_name_value, 85, 64);
	info_fixed->put(*file_size_label, 8, 80);
	info_fixed->put(*file_size_value, 85, 80);
	info_fixed->put(*file_percent_label, 8, 96);
	info_fixed->put(*file_percent_value, 85, 96);
	info_fixed->put(*total_speed_label, 8, 128);
	info_fixed->put(*total_speed_value, 85, 128);
	info_fixed->put(*servers_connected_label, 8, 144);
	info_fixed->put(*servers_connected_value, 85, 144);

	//server info setup
	Gtk::TreeModel::ColumnRecord column;
	Gtk::TreeModelColumn<Glib::ustring> servers_t;
	Gtk::TreeModelColumn<Glib::ustring> speed_t;

	column.add(servers_t);
	column.add(speed_t);

	servers_list = Gtk::ListStore::create(column);
	servers_view->set_model(servers_list);

	//add columns
	servers_view->append_column(" IP ", servers_t);
	servers_view->append_column(" Speed ", speed_t);

	show_all_children();
}

bool gui_download_status::refresh()
{
	std::vector<download_info> info;
	Client->current_downloads(info, root_hash);

	if(info.empty()){
		//download finished
		servers_list->clear();
		return false;
	}

	//update top pane info
	std::ostringstream percent_complete_oss;
	percent_complete_oss << info.begin()->percent_complete << "%";

	if(info.begin()->size == tree_size_bytes){
		//download in download_hash_tree stage
		hash_tree_percent_value->set_text(percent_complete_oss.str());
	}else{
		//download in donwload_file_stage
		hash_tree_percent_value->set_text("100%");
		file_percent_value->set_text(percent_complete_oss.str());
	}

	std::ostringstream total_speed_oss;
	total_speed_oss << convert::size_unit_select(info.begin()->total_speed) << "/s";
	total_speed_value->set_text(total_speed_oss.str());

	std::ostringstream servers_connected_oss;
	servers_connected_oss << info.begin()->IP.size();
	servers_connected_value->set_text(servers_connected_oss.str());

	//update bottom pane info
	assert(info.begin()->IP.size() == info.begin()->speed.size());
	for(int x=0; x<info.begin()->IP.size(); ++x){
		std::string IP = info.begin()->IP[x];
		unsigned int speed = info.begin()->speed[x];

		//set up column
		Gtk::TreeModel::ColumnRecord column;
		Gtk::TreeModelColumn<Glib::ustring> servers_t;
		Gtk::TreeModelColumn<Glib::ustring> speed_t;

		column.add(servers_t);
		column.add(speed_t);

		//attempt to locate existing entry in treeview
		bool entry_found = false;
		Gtk::TreeModel::Children children = servers_list->children();
		Gtk::TreeModel::Children::iterator iter_cur, iter_end;
		iter_cur = children.begin();
		iter_end = children.end();
		while(iter_cur != iter_end){
 			Gtk::TreeModel::Row row = *iter_cur;
			Glib::ustring IP_retrieved;
			row.get_value(0, IP_retrieved);
			if(IP_retrieved == IP){
				row[speed_t] = convert::size_unit_select(speed) + "/s";
				entry_found = true;
				break;
			}
			++iter_cur;
		}

		if(!entry_found){
			Gtk::TreeModel::Row row = *(servers_list->append());
			row[servers_t] = IP;
			row[speed_t] = convert::size_unit_select(speed) + "/s";
		}
	}

	//remove rows without a corresponding server in the download_info element
	Gtk::TreeModel::Children children = servers_list->children();
	Gtk::TreeModel::Children::iterator iter_cur, iter_end;
	iter_cur = children.begin();
	iter_end = children.end();
	while(iter_cur != iter_end){
	 	Gtk::TreeModel::Row row = *iter_cur;
		Glib::ustring IP_retrieved;
		row.get_value(0, IP_retrieved);

		bool entry_found(false);
		for(int x=0; x<info.begin()->IP.size(); ++x){
			std::string IP = info.begin()->IP[x];
			if(IP_retrieved == IP){
				entry_found = true;
				break;
			}
		}

		if(!entry_found){
			iter_cur = servers_list->erase(iter_cur);
		}else{
			++iter_cur;
		}
	}

	return true;
}
