#include "gui_download_status.h"

gui_download_status::gui_download_status()
{
	window = this;
	window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);

	vbox = Gtk::manage(new Gtk::VBox(false, 0));
	info_fixed = Gtk::manage(new Gtk::Fixed);
	servers_scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	servers_tree_view = Gtk::manage(new Gtk::TreeView);
	root_hash = Gtk::manage(new Gtk::Label("Root Hash: "));
	root_hash_value = Gtk::manage(new Gtk::Label("ABCDEFABCDEFABCDEFAB"));
	hash_tree_size = Gtk::manage(new Gtk::Label("Tree Size: "));
	hash_tree_size_value = Gtk::manage(new Gtk::Label("2mB"));
	hash_tree_percent = Gtk::manage(new Gtk::Label("Complete: "));
	hash_tree_percent_value = Gtk::manage(new Gtk::Label("0%"));
	file_name = Gtk::manage(new Gtk::Label("File Name: "));
	file_name_value = Gtk::manage(new Gtk::Label("Test.avi"));
	file_size = Gtk::manage(new Gtk::Label("File Size: "));
	file_size_value = Gtk::manage(new Gtk::Label("700mB"));
	file_percent = Gtk::manage(new Gtk::Label("Complete: "));
	file_percent_value = Gtk::manage(new Gtk::Label("0%"));
	total_speed = Gtk::manage(new Gtk::Label("Speed: "));
	total_speed_value = Gtk::manage(new Gtk::Label("69kB/s"));
	servers_connected = Gtk::manage(new Gtk::Label("Servers: "));
	servers_connected_value = Gtk::manage(new Gtk::Label("3"));

	add(*vbox);
	vbox->pack_start(*info_fixed, Gtk::PACK_SHRINK, 8);
	vbox->pack_start(*servers_scrolled_window);

	servers_scrolled_window->add(*servers_tree_view);
	servers_scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);

	info_fixed->put(*root_hash, 8, 0);
	info_fixed->put(*root_hash_value, 85, 0);
	info_fixed->put(*hash_tree_size, 8, 16);
	info_fixed->put(*hash_tree_size_value, 85, 16);
	info_fixed->put(*hash_tree_percent, 8, 32);
	info_fixed->put(*hash_tree_percent_value, 85, 32);
	info_fixed->put(*file_name, 8, 64);
	info_fixed->put(*file_name_value, 85, 64);
	info_fixed->put(*file_size, 8, 80);
	info_fixed->put(*file_size_value, 85, 80);
	info_fixed->put(*file_percent, 8, 96);
	info_fixed->put(*file_percent_value, 85, 96);
	info_fixed->put(*total_speed, 8, 128);
	info_fixed->put(*total_speed_value, 85, 128);
	info_fixed->put(*servers_connected, 8, 144);
	info_fixed->put(*servers_connected_value, 85, 144);

	show_all_children();
}

void gui_download_status::refresh()
{

}
