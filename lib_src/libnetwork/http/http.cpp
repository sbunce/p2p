#include "http.hpp"

http::http(
	const std::string & web_root_in
):
	Proactor(
		boost::bind(&http::connect_call_back, this, _1),
		boost::bind(&http::disconnect_call_back, this, _1)
	),
	web_root(web_root_in)
{
	//no need to have a limit on outgoing connections
	Proactor.set_connection_limit(1000, 0);
}

http::~http()
{
	Proactor.stop();
}

void http::connect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID,
		new connection(Proactor, CI, web_root)));
	assert(ret.second);
}

void http::disconnect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(CI.connection_ID);
}

std::string http::port()
{
	return Proactor.listen_port();
}

void http::set_max_upload_rate(const unsigned rate)
{
	Proactor.set_max_upload_rate(rate);
}

void http::start(const std::string & port, const bool localhost_only)
{
	std::set<network::endpoint> E = network::get_endpoint(
		localhost_only ? "localhost" : "",
		port,
		network::tcp
	);
	assert(!E.empty());
	Proactor.start(*E.begin());
}
