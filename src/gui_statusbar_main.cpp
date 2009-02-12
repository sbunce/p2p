#include "gui_statusbar_main.hpp"

gui_statusbar_main::gui_statusbar_main(
	client & Client_in,
	server & Server_in
):
	Client(&Client_in),
	Server(&Server_in)
{
	statusbar = this;
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &gui_statusbar_main::update_status_bar), global::GUI_TICK);
}

bool gui_statusbar_main::update_status_bar()
{
	std::string status; //holds entire status line

	//get the total client download speed
	int client_rate = Client->total_rate();
	std::stringstream client_rate_s;
	client_rate_s << convert::size_SI(client_rate) << "/s";

	//get the total server upload speed
	int server_rate = Server->total_rate();
	std::stringstream server_rate_s;
	server_rate_s << convert::size_SI(server_rate) << "/s";

	std::stringstream ss;
	ss << " D: " << client_rate_s.str();
	ss << std::string(16 - ss.str().size(), ' ');
	ss << " U: " << server_rate_s.str();
	ss << std::string(32 - ss.str().size(), ' ');
	ss << " P: " << Client->prime_count();
	ss << std::string(42 - ss.str().size(), ' ');

	if(Server->is_indexing()){
		ss << "  Indexing/Hashing";
	}

	statusbar->pop();
	statusbar->push(ss.str());

	return true;
}
