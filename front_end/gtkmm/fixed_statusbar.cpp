#include "fixed_statusbar.hpp"

fixed_statusbar::fixed_statusbar(
	p2p & P2P_in
):
	P2P(P2P_in)
{
	put(download_label, 5, 0);
	put(upload_label, 100, 0);

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &fixed_statusbar::update), settings::GUI_TICK);
}

bool fixed_statusbar::update()
{
	std::stringstream ss;
	ss << "D: " << convert::bytes_to_SI(P2P.TCP_download_rate() + P2P.UDP_download_rate()) << "/s";
	download_label.set_label(ss.str());

	ss.str(""); ss.clear();
	ss << "U: " << convert::bytes_to_SI(P2P.TCP_upload_rate() + P2P.UDP_upload_rate()) << "/s";
	upload_label.set_label(ss.str());

	return true;
}
