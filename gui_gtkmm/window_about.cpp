#include "window_about.hpp"

//returns the version of the program (version based on compile date)
std::string version()
{
	std::string year(__DATE__, 7, 4);
	std::string month(__DATE__, 0, 3);
	if(month == "Jan"){
		month = "01";
	}else if(month == "Feb"){
		month = "02";
	}else if(month == "Mar"){
		month = "03";
	}else if(month == "Apr"){
		month = "04";
	}else if(month == "May"){
		month = "05";
	}else if(month == "Jun"){
		month = "06";
	}else if(month == "Jul"){
		month = "07";
	}else if(month == "Aug"){
		month = "08";
	}else if(month == "Sep"){
		month = "09";
	}else if(month == "Oct"){
		month = "10";
	}else if(month == "Nov"){
		month = "11";
	}else if(month == "Dec"){
		month = "12";
	}else{
		LOG << "unknown month";
		exit(1);
	}
	std::string day(__DATE__, 4, 2);
	if(day[0] == ' '){
		day[0] = '0';
	}
	return year + month + day;
}

window_about::window_about()
{
	window = this;

	vbox = Gtk::manage(new Gtk::VBox(false, 0));
	scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
	text_view = Gtk::manage(new Gtk::TextView);
	button_box = Gtk::manage(new Gtk::HButtonBox);
	close_button = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));

	window->set_icon(
		Gtk::Widget::render_icon(Gtk::Stock::ABOUT, Gtk::ICON_SIZE_LARGE_TOOLBAR)
	);

	window->set_title(("About P2P"));
	window->resize(500, 350);
	window->set_modal(true);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER);

	add(*vbox);
	scrolled_window->add(*text_view);
	scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*scrolled_window);


	text_buff = Gtk::TextBuffer::create();
	text_buff->set_text(settings::NAME+" version: "+version()+" © Seth Bunce 2006");
	text_view->set_buffer(text_buff);
	text_view->set_editable(false);

	vbox->pack_start(*button_box, Gtk::PACK_SHRINK);
	button_box->pack_start(*close_button, Gtk::PACK_SHRINK);

	//signaled functions
	close_button->signal_clicked().connect(sigc::mem_fun(*this, &window_about::close_click));

	show_all_children();
}

void window_about::close_click()
{
	hide();
}
