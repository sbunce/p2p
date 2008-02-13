#include "global.h"
#include "gui_about.h"

gui_about::gui_about()
{
	window = this;

	window->set_title(("about"));
	window->resize(500, 350);
	window->set_modal(false);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER_ALWAYS);

	add(about_VBox);
	about_scrolledWindow.add(about_textView);
	about_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	about_VBox.pack_start(about_scrolledWindow);
	about_refTextBuff = Gtk::TextBuffer::create();

	about_refTextBuff->set_text(
	global::NAME + " version: " + global::VERSION + " © Seth Bunce 2006\n\n\n" +

	"Acknowledgements:\n\n" +

	"SHA C++ Implementation\n" +
	"  Keith Oxenrider  - http://sol-biotech.com/code\n" +
	"  Aaron D. Gifford - http://www.aarongifford.com/computers/sha.html\n\n" +

	"SQLite Embeddable SQL DB\n" +
	"  Richard Hipp - http://www.hwaci.com/drh/\n\n\n"
	);

	about_textView.set_buffer(about_refTextBuff);

	about_VBox.pack_start(about_buttonBox, Gtk::PACK_SHRINK);
	about_buttonBox.pack_start(buttonClose, Gtk::PACK_SHRINK);

	buttonClose.set_label("Close");
	buttonClose.signal_clicked().connect(sigc::mem_fun(*this,
		&gui_about::on_button_close));

	show_all_children();
}

void gui_about::on_button_close()
{
	hide();
}
