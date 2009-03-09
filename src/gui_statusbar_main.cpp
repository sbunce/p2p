#include "gui_statusbar_main.hpp"

gui_statusbar_main::gui_statusbar_main(
	p2p & P2P_in
):
	P2P(&P2P_in)
{
	statusbar = this;
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui_statusbar_main::update_status_bar), settings::GUI_TICK);
}

bool gui_statusbar_main::update_status_bar()
{
	std::string status; //holds entire status line

	//get the total client download speed
	int download_rate = P2P->download_rate();
	std::stringstream download_rate_s;
	download_rate_s << convert::size_SI(download_rate) << "/s";

	//get the total server upload speed
	int upload_rate = P2P->upload_rate();
	std::stringstream upload_rate_s;
	upload_rate_s << convert::size_SI(upload_rate) << "/s";

	std::stringstream ss;
	ss << " D: " << download_rate_s.str();
	ss << std::string(16 - ss.str().size(), ' ');
	ss << " U: " << upload_rate_s.str();
	ss << std::string(32 - ss.str().size(), ' ');
	ss << " P: " << P2P->prime_count();
	ss << std::string(42 - ss.str().size(), ' ');

	if(P2P->is_indexing()){
		ss << "  Indexing/Hashing";
	}

	statusbar->pop();
	statusbar->push(ss.str());

	return true;
}
