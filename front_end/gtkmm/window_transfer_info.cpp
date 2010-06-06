#include "window_transfer_info.hpp"

window_transfer_info::window_transfer_info(
	p2p & P2P_in,
	const p2p::transfer_info & TI
):
	P2P(P2P_in),
	hash(TI.hash)
{
	Gtk::VBox * vbox = Gtk::manage(new Gtk::VBox(false, 0));
	Gtk::Fixed * info_fixed = Gtk::manage(new Gtk::Fixed);
	Gtk::Label * hash_label = Gtk::manage(new Gtk::Label("Hash: "));
	Gtk::Label * hash_value = Gtk::manage(new Gtk::Label(TI.hash));
	Gtk::Label * tree_size_label = Gtk::manage(new Gtk::Label("Tree Size: "));
	Gtk::Label * tree_percent_label = Gtk::manage(new Gtk::Label("Complete: "));
	Gtk::Label * file_name_label = Gtk::manage(new Gtk::Label("File Name: "));
	Gtk::Label * file_size_label = Gtk::manage(new Gtk::Label("File Size: "));
	Gtk::Label * file_percent_label = Gtk::manage(new Gtk::Label("Complete: "));
	Gtk::Label * download_speed_label = Gtk::manage(new Gtk::Label("Down: "));
	Gtk::Label * upload_speed_label = Gtk::manage(new Gtk::Label("Up: "));

	tree_size_value = Gtk::manage(new Gtk::Label(convert::bytes_to_SI(TI.tree_size)));
	tree_percent_value = Gtk::manage(new Gtk::Label("0%"));
	file_name_value = Gtk::manage(new Gtk::Label(TI.name));
	file_size_value = Gtk::manage(new Gtk::Label(convert::bytes_to_SI(TI.file_size)));
	file_percent_value = Gtk::manage(new Gtk::Label("0%"));
	download_speed_value = Gtk::manage(new Gtk::Label(convert::bytes_to_SI(TI.download_speed)+"/s"));
	upload_speed_value = Gtk::manage(new Gtk::Label(convert::bytes_to_SI(TI.upload_speed)+"/s"));
	servers_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	servers_view = Gtk::manage(new Gtk::TreeView);

	//add/compose elements
	this->add(*vbox);
	vbox->pack_start(*info_fixed, Gtk::PACK_SHRINK, 8);
	vbox->pack_start(*servers_scrolled_window);
	servers_scrolled_window->add(*servers_view);
	info_fixed->put(*file_name_label, 8, 0);
	info_fixed->put(*file_name_value, 90, 0);
	info_fixed->put(*hash_label, 8, 16);
	info_fixed->put(*hash_value, 90, 16);
	info_fixed->put(*tree_size_label, 8, 32);
	info_fixed->put(*tree_size_value, 90, 32);
	info_fixed->put(*tree_percent_label, 160 + 8, 32);
	info_fixed->put(*tree_percent_value, 160 + 90, 32);
	info_fixed->put(*file_size_label, 8, 48);
	info_fixed->put(*file_size_value, 90, 48);
	info_fixed->put(*file_percent_label, 160 + 8, 48);
	info_fixed->put(*file_percent_value, 160 + 90, 48);
	info_fixed->put(*download_speed_label, 8, 64);
	info_fixed->put(*download_speed_value, 90, 64);
	info_fixed->put(*upload_speed_label, 160 + 8, 64);
	info_fixed->put(*upload_speed_value, 160 + 90, 64);

	//options
	this->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC); //auto scroll bars
	servers_scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

/*
	//setup servers_view
	column.add(column_server);
	column.add(column_speed);
	servers_list = Gtk::ListStore::create(column);
	servers_view->set_model(servers_list);
	servers_view->append_column(" IP ", column_server);
	servers_view->append_column(" Speed ", column_speed);
*/
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &window_transfer_info::refresh),
		settings::GUI_tick);

	this->show_all_children();
}

bool window_transfer_info::refresh()
{
	boost::optional<p2p::transfer_info> TI = P2P.transfer(hash);
	if(TI){
		std::stringstream ss;
		tree_size_value->set_text(convert::bytes_to_SI(TI->tree_size));
		ss << TI->tree_percent_complete << "%";
		tree_percent_value->set_text(ss.str());
		file_size_value->set_text(convert::bytes_to_SI(TI->file_size));
		ss.str(""); ss.clear();
		ss << TI->file_percent_complete << "%";
		file_percent_value->set_text(ss.str());
		download_speed_value->set_text(convert::bytes_to_SI(TI->download_speed)+"/s");
		upload_speed_value->set_text(convert::bytes_to_SI(TI->upload_speed)+"/s");
		return true;
	}else{
		download_speed_value->set_text("0B/s");
		upload_speed_value->set_text("0B/s");
		return false;
	}
}
