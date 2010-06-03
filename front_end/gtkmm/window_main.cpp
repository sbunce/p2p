#include "window_main.hpp"

window_main::window_main():
	Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{
	window = this;

	menubar = Gtk::manage(new Gtk::MenuBar);
	notebook = Gtk::manage(new Gtk::Notebook);
/*
	//menus
	file_menu = Gtk::manage(new Gtk::Menu);
	settings_menu = Gtk::manage(new Gtk::Menu);
	help_menu = Gtk::manage(new Gtk::Menu);
*/
	//notebook labels
	Gtk::Image * download_label = Gtk::manage(new Gtk::Image(Gtk::Stock::GO_DOWN,
		Gtk::ICON_SIZE_LARGE_TOOLBAR));
	Gtk::Image * upload_label = Gtk::manage(new Gtk::Image(Gtk::Stock::GO_UP,
		Gtk::ICON_SIZE_LARGE_TOOLBAR));

	Window_Download = Gtk::manage(new window_transfer(P2P, window_transfer::download));
	Window_Upload = Gtk::manage(new window_transfer(P2P, window_transfer::upload));
	statusbar = Gtk::manage(new fixed_statusbar(P2P));

	//boxes (divides the window)
	main_VBox = Gtk::manage(new Gtk::VBox(false, 0));

	//icon for top left of window
	window->set_icon(
		Gtk::Widget::render_icon(Gtk::Stock::NETWORK, Gtk::ICON_SIZE_LARGE_TOOLBAR)
	);
/*
	//add items to File menu
	file_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::QUIT)));
	quit = static_cast<Gtk::ImageMenuItem *>(&file_menu->items().back());
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_File"), *file_menu));
	file_menu_item = static_cast<Gtk::MenuItem *>(&menubar->items().back());
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Settings"), *settings_menu));
	settings_menu_item = static_cast<Gtk::MenuItem *>(&menubar->items().back());
	menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(("_Help"), *help_menu));
	help_menu_item = static_cast<Gtk::MenuItem *>(&menubar->items().back());

	//add items to Settings menu
	settings_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(
		Gtk::StockID(Gtk::Stock::PREFERENCES)));
	preferences = static_cast<Gtk::MenuItem *>(&settings_menu->items().back());

	//add items to Help menu
	help_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID(Gtk::Stock::ABOUT)));
	about = static_cast<Gtk::MenuItem *>(&help_menu->items().back());
*/
	//set notebook properties
	notebook->set_flags(Gtk::CAN_FOCUS);
	notebook->set_show_tabs(true);
	notebook->set_show_border(true);
	notebook->set_tab_pos(Gtk::POS_TOP);
	notebook->set_scrollable(true);

	//add elements to the notebook
	notebook->append_page(*Window_Download, *download_label);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	notebook->append_page(*Window_Upload, *upload_label);
	notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);

	//add items to the main VBox
	//main_VBox->pack_start(*menubar, Gtk::PACK_SHRINK, 0);
	main_VBox->pack_start(*notebook);
	main_VBox->pack_start(*statusbar, Gtk::PACK_SHRINK, 0);

	//window properties
	window->set_title(settings::name);
	window->resize(640, 480);
	window->set_modal(false);
	window->property_window_position().set_value(Gtk::WIN_POS_CENTER_ON_PARENT);
	window->set_resizable(true);
	window->property_destroy_with_parent().set_value(false);
	window->add(*main_VBox);

	//set up window to get URIs of files dropped on it
	list_targets.push_back(Gtk::TargetEntry("STRING"));
	list_targets.push_back(Gtk::TargetEntry("text/plain"));
	window->drag_dest_set(list_targets);

	//signaled functions
	window->signal_drag_data_received().connect(sigc::mem_fun(*this, &window_main::file_drag_data_received));
	//quit->signal_activate().connect(sigc::mem_fun(*this, &window_main::on_quit), false);
	//preferences->signal_activate().connect(sigc::mem_fun(*this, &window_main::settings_preferences), false);
	//about->signal_activate().connect(sigc::mem_fun(*this, &window_main::help_about), false);

	//set objects to be visible
	window->show_all_children();
}

window_main::~window_main()
{

}

void window_main::file_drag_data_received(
	const Glib::RefPtr<Gdk::DragContext> & context, //info about drag (available on source and dest)
	int x,                                          //x-coord of drop
	int y,                                          //y-coord of drop
	const Gtk::SelectionData & selection_data,      //data dropped
	guint info,                                     //type of info dropped
	guint time)                                     //time stamp of when drop happened
{
	if(selection_data.get_length() >= 0 && selection_data.get_format() == 8){
		process_URI(selection_data.get_data_as_string());
	}

	//tell other end we're done with drag and drop so it can free resources
	context->drag_finish(true, true, time);
}

void window_main::help_about()
{
	boost::shared_ptr<window_about> Window_About(new window_about());
	Gtk::Main::run(*Window_About);
}

void window_main::on_quit()
{
	hide();
}

bool window_main::on_delete_event(GdkEventAny * event)
{
	on_quit();
	return true;
}

void window_main::process_URI(const std::string & URI)
{
	LOG << URI;
}

void window_main::settings_preferences()
{
	boost::shared_ptr<window_prefs> prefs(new window_prefs(P2P));
	Gtk::Main::run(*prefs);
}
