#include "gui_window_download_status.hpp"

gui_window_download_status::gui_window_download_status(
	const std::string & root_hash_in,
	const std::string & path,
	const boost::uint64_t & tree_size_in,
	const boost::uint64_t & file_size_in,
	Gtk::HBox *& hbox,
	Gtk::Label *& tab_label,
	Gtk::Button *& close_button,
	p2p & P2P_in
):
	root_hash(root_hash_in),
	tree_size(tree_size_in),
	P2P(&P2P_in)
{
	window = this;
	window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);

	std::string tab_label_text, full_name;
	if(path.find_last_of('/') == std::string::npos){
		full_name = path;
		tab_label_text = path;
	}else{
		full_name = path.substr(path.find_last_of('/')+1);
		tab_label_text = full_name;
	}

	//shorten long names to make tabs reasonable max width
	if(tab_label_text.size() > 24){
		tab_label_text = "  " + tab_label_text.substr(0, 24) + "..  ";
	}

	//instantiation of tab label related
	hbox = Gtk::manage(new Gtk::HBox(false, 0));
	tab_label = Gtk::manage(new Gtk::Label(tab_label_text));
	close_button = Gtk::manage(new Gtk::Button);
	close_image = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU));

	hbox->pack_start(*tab_label);
	close_button->add(*close_image);
	hbox->pack_start(*close_button);
	hbox->show_all_children();

	//instantiation of the rest
	vbox = Gtk::manage(new Gtk::VBox(false, 0));
	info_fixed = Gtk::manage(new Gtk::Fixed);
	servers_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	servers_view = Gtk::manage(new Gtk::TreeView);
	root_hash_label = Gtk::manage(new Gtk::Label("Root Hash: "));
	root_hash_value = Gtk::manage(new Gtk::Label(root_hash_in));
	hash_tree_size_label = Gtk::manage(new Gtk::Label("Tree Size: "));
	hash_tree_size_value = Gtk::manage(new Gtk::Label(convert::size_SI(tree_size)));
	hash_tree_percent_label = Gtk::manage(new Gtk::Label("Complete: "));
	hash_tree_percent_value = Gtk::manage(new Gtk::Label("0%"));
	file_name_label = Gtk::manage(new Gtk::Label("File Name: "));
	file_name_value = Gtk::manage(new Gtk::Label(full_name));
	file_size_label = Gtk::manage(new Gtk::Label("File Size: "));
	file_size_value = Gtk::manage(new Gtk::Label(convert::size_SI(file_size_in)));
	file_percent_label = Gtk::manage(new Gtk::Label("Complete: "));
	file_percent_value = Gtk::manage(new Gtk::Label("0%"));
	total_speed_label = Gtk::manage(new Gtk::Label("Speed: "));
	total_speed_value = Gtk::manage(new Gtk::Label);
	servers_connected_label = Gtk::manage(new Gtk::Label("Servers: "));
	servers_connected_value = Gtk::manage(new Gtk::Label);

	window->add(*vbox);
	vbox->pack_start(*info_fixed, Gtk::PACK_SHRINK, 8);
	vbox->pack_start(*servers_scrolled_window);
	servers_scrolled_window->add(*servers_view);
	servers_scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	info_fixed->put(*root_hash_label, 8, 0);
	info_fixed->put(*root_hash_value, 90, 0);
	info_fixed->put(*hash_tree_size_label, 8, 16);
	info_fixed->put(*hash_tree_size_value, 90, 16);
	info_fixed->put(*hash_tree_percent_label, 8, 32);
	info_fixed->put(*hash_tree_percent_value, 90, 32);
	info_fixed->put(*file_name_label, 8, 64);
	info_fixed->put(*file_name_value, 90, 64);
	info_fixed->put(*file_size_label, 8, 80);
	info_fixed->put(*file_size_value, 90, 80);
	info_fixed->put(*file_percent_label, 8, 96);
	info_fixed->put(*file_percent_value, 90, 96);
	info_fixed->put(*total_speed_label, 8, 128);
	info_fixed->put(*total_speed_value, 90, 128);
	info_fixed->put(*servers_connected_label, 8, 144);
	info_fixed->put(*servers_connected_value, 90, 144);

	//setup servers_view
	column.add(column_server);
	column.add(column_speed);
	servers_list = Gtk::ListStore::create(column);
	servers_view->set_model(servers_list);
	servers_view->append_column(" IP ", column_server);
	servers_view->append_column(" Speed ", column_speed);
}

bool gui_window_download_status::refresh()
{
	std::vector<download_status> info;
	P2P->current_downloads(info, root_hash);

	if(info.empty()){
		//download finished
		servers_list->clear();
		total_speed_value->set_text("0 B/s");
		servers_connected_value->set_text("0");

		/*
		Keep refreshing even if there is no info. It is possible that the download
		is transitioning and there is no info at this time.
		*/
		return true;
	}

	//update top pane info
	std::stringstream percent_complete_ss;
	percent_complete_ss << info.begin()->percent_complete << "%";

	if(info.begin()->size == tree_size){
		//download in download_hash_tree stage
		hash_tree_percent_value->set_text(percent_complete_ss.str());
	}else{
		//download in donwload_file_stage
		hash_tree_percent_value->set_text("100%");
		file_percent_value->set_text(percent_complete_ss.str());
	}

	std::stringstream ss;
	ss << convert::size_SI(info.begin()->total_speed) << "/s";
	total_speed_value->set_text(ss.str());

	ss.str(""); ss.clear();
	ss << info.begin()->IP.size();
	servers_connected_value->set_text(ss.str());

	//update bottom pane info
	assert(info.begin()->IP.size() == info.begin()->speed.size());
	for(int x=0; x<info.begin()->IP.size(); ++x){
		std::string IP = info.begin()->IP[x];
		unsigned speed = info.begin()->speed[x];

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
				row[column_speed] = convert::size_SI(speed) + "/s";
				entry_found = true;
				break;
			}
			++iter_cur;
		}
		if(!entry_found){
			Gtk::TreeModel::Row row = *(servers_list->append());
			row[column_server] = IP;
			row[column_speed] = convert::size_SI(speed) + "/s";
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
