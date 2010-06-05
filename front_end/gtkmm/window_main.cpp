#include "window_main.hpp"

window_main::window_main():
	Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{
	//splits main window
	Gtk::VBox * VBox = Gtk::manage(new Gtk::VBox(false, 0));
	//notebook and tab labels
	Gtk::Notebook * notebook = Gtk::manage(new Gtk::Notebook);
	Gtk::Image * download_label = Gtk::manage(new Gtk::Image(Gtk::Stock::GO_DOWN,
		Gtk::ICON_SIZE_LARGE_TOOLBAR));
	Gtk::Image * upload_label = Gtk::manage(new Gtk::Image(Gtk::Stock::GO_UP,
		Gtk::ICON_SIZE_LARGE_TOOLBAR));
	Gtk::Image * prefs_label = Gtk::manage(new Gtk::Image(Gtk::Stock::PREFERENCES,
		Gtk::ICON_SIZE_LARGE_TOOLBAR));
	//windows inside tabs
	window_transfer * download_window = Gtk::manage(new window_transfer(
		window_transfer::download, P2P, notebook));
	window_transfer * upload_window = Gtk::manage(new window_transfer(
		window_transfer::upload, P2P, notebook));
	window_prefs * prefs_window = Gtk::manage(new window_prefs(P2P));
	//bar on bottom of window (that lists upload/download speeds)
	Gtk::Fixed * statusbar = Gtk::manage(new fixed_statusbar(P2P));

	//icon for top left of window
	this->set_icon(Gtk::Widget::render_icon(Gtk::Stock::NETWORK, Gtk::ICON_SIZE_LARGE_TOOLBAR));

	//add/compose elements
	notebook->append_page(*download_window, *download_label);
	notebook->append_page(*upload_window, *upload_label);
	notebook->append_page(*prefs_window, *prefs_label);
	VBox->pack_start(*notebook);
	VBox->pack_start(*statusbar, Gtk::PACK_SHRINK, 0);
	this->add(*VBox);

	//options
	notebook->set_scrollable(true);  //scroll when many tabs open
	this->set_title(settings::name);
	this->resize(640, 480);
	this->set_resizable(true);
	this->property_destroy_with_parent().set_value(false);

	//get URIs of files dropped on it
	std::list<Gtk::TargetEntry> list_targets;
	list_targets.push_back(Gtk::TargetEntry("STRING"));
	list_targets.push_back(Gtk::TargetEntry("text/plain"));
	this->drag_dest_set(list_targets);

	//signaled functions
	this->signal_drag_data_received().connect(sigc::mem_fun(*this,
		&window_main::file_drag_data_received));

	this->show_all_children();
}

void window_main::file_drag_data_received(
	const Glib::RefPtr<Gdk::DragContext> & context, //info about drag (available on source and dest)
	const int x,                                    //x-coord of drop
	const int y,                                    //y-coord of drop
	const Gtk::SelectionData & selection_data,      //data dropped
	const guint info,                               //type of info dropped
	const guint time)                               //time stamp of when drop happened
{
//DEBUG, this does not work on windows, not sure why

	if(selection_data.get_length() >= 0 && selection_data.get_format() == 8){
		process_URI(selection_data.get_data_as_string());
	}

	//tell other end we're done with drag and drop so it can free resources
	context->drag_finish(true, true, time);
}

bool window_main::on_delete_event(GdkEventAny * event)
{
	hide();
	return true;
}

void window_main::process_URI(const std::string & URI)
{
	LOG << URI;
}
