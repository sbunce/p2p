#include "statusbar_main.hpp"

statusbar_main::statusbar_main(
	p2p & P2P_in
):
	P2P(P2P_in)
{
	statusbar = this;
	Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&statusbar_main::update_status_bar), settings::GUI_TICK);
}

bool statusbar_main::update_status_bar()
{
	std::stringstream ss;
	ss
	<< "TCP("
		<< "D: " << convert::bytes_to_SI(P2P.TCP_download_rate()) << "/s "
		<< "U: " << convert::bytes_to_SI(P2P.TCP_upload_rate()) << "/s"
	<< ") "
	<< "UDP("
		<< "D: " << convert::bytes_to_SI(P2P.UDP_download_rate()) << "/s "
		<< "U: " << convert::bytes_to_SI(P2P.UDP_upload_rate()) << "/s"
	<< ") "
	<< "DHT: " << P2P.DHT_count() << " "
	<< "S: " << convert::bytes_to_SI(P2P.share_size_bytes()) << " ("
		<< P2P.share_size_files() << ")";

	statusbar->pop();
	statusbar->push(ss.str());
	return true;
}
