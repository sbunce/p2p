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
	//get the total client download speed
	int download_rate = P2P.download_rate();
	std::stringstream download_rate_s;
	download_rate_s << convert::size_SI(download_rate) << "/s";

	//get the total server upload speed
	int upload_rate = P2P.upload_rate();
	std::stringstream upload_rate_s;
	upload_rate_s << convert::size_SI(upload_rate) << "/s";


	std::stringstream ss;
	ss << " DL: " << download_rate_s.str() << " ";
	ss << " UL: " << upload_rate_s.str() << " ";
	ss << " Prime: " << P2P.prime_count() << " ";
	ss << " Share: " << convert::size_SI(P2P.share_size_bytes()) << " ";
	ss << " Share: " << P2P.share_size_files();

	statusbar->pop();
	statusbar->push(ss.str());
	return true;
}
