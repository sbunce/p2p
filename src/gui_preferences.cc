//custom
#include "global.h"

#include "gui_preferences.h"

gui_preferences::gui_preferences()
{
	window = this;

	window->set_title(("Preferences"));
	window->resize(500, 350);
	window->set_modal(false);
	window->set_keep_above(true);
	window->set_position(Gtk::WIN_POS_CENTER_ALWAYS);

	add(preferences_VBox);

	preferences_VBox.pack_start(buttonBox, Gtk::PACK_SHRINK);
	buttonBox.pack_start(buttonClose, Gtk::PACK_SHRINK);
	buttonBox.pack_start(buttonApply, Gtk::PACK_SHRINK);

	buttonClose.set_label("Close");
	buttonApply.set_label("Apply");

	//set objects to be visible
	show_all_children();

	//signaled functions
	buttonClose.signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::button_close));
	buttonApply.signal_clicked().connect(sigc::mem_fun(*this, &gui_preferences::button_apply));
}

void gui_preferences::button_close()
{
	hide();
}

void gui_preferences::button_apply()
{
	std::cout << "ZORT\n";
}
