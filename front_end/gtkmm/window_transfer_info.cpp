#include "window_transfer_info.hpp"

window_transfer_info::window_transfer_info(
	const type_t type_in,
	p2p & P2P_in,
	const std::string & hash_in
):
	type(type_in),
	P2P(P2P_in),
	hash(hash_in)
{
	Gtk::VBox * VBox = Gtk::manage(new Gtk::VBox(false, 0));
	Gtk::Fixed * top_fixed = Gtk::manage(new Gtk::Fixed);

	//add/compose elements
	VBox->pack_start(*top_fixed);
}
