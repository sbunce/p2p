#include "http.hpp"

http::http(
	const std::string & web_root_in,
	const std::string & port_in,
	const bool localhost_only_in
):
	web_root(web_root_in),
	port(port_in),
	localhost_only(localhost_only_in),
	Proactor(
		boost::bind(&http::connect_call_back, this, _1),
		boost::bind(&http::disconnect_call_back, this, _1)
	)
{
	//no need to have a limit on outgoing connections
	Proactor.set_connection_limit(1000, 0);
}

http::~http()
{
	Proactor.stop();
}

void http::connect_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID,
		new connection(Proactor, CI, web_root)));
	assert(ret.second);
}

void http::disconnect_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(CI.connection_ID);
}

void http::set_max_upload_rate(const unsigned rate)
{
	Proactor.set_max_upload_rate(rate);
}

void http::start(boost::shared_ptr<net::listener> Listener)
{
	boost::mutex::scoped_lock lock(start_stop_mutex);
	assert(Listener);
	Proactor.start(Listener);
}

void http::stop()
{
	boost::mutex::scoped_lock lock(start_stop_mutex);
	Proactor.stop();
}
