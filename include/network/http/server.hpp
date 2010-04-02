#ifndef H_NETWORK_HTTP_SERVER
#define H_NETWORK_HTTP_SERVER

//custom
#include "connection.hpp"
#include "../proactor.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace network{
namespace http{
class server
{
public:
	server(
		const std::string & web_root_in,
		bool localhost_only = false
	):
		Proactor(
			boost::bind(&server::connect_call_back, this, _1),
			boost::bind(&server::disconnect_call_back, this, _1)
		),
		web_root(web_root_in)
	{
		std::set<network::endpoint> E = network::get_endpoint(
			localhost_only ? "localhost" : "",
			"8080",
			network::tcp
		);
		assert(!E.empty());
		Proactor.start(*E.begin());
	}

	~server()
	{
LOGGER;
		Proactor.stop();
LOGGER;
	}

	//return port server is listening on
	std::string port()
	{
		return Proactor.listen_port();
	}

private:
	network::proactor Proactor;
	std::string web_root;

	//connection specific information
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;

	//proactor calls back to this when connection happens
	void connect_call_back(network::connection_info & CI)
	{
		boost::mutex::scoped_lock lock(Connection_mutex);
		std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
			ret = Connection.insert(std::make_pair(CI.connection_ID,
			new connection(Proactor, CI, web_root)));
		assert(ret.second);
	}

	//proactor calls back to this when disconnect
	void disconnect_call_back(network::connection_info & CI)
	{
		boost::mutex::scoped_lock lock(Connection_mutex);
		Connection.erase(CI.connection_ID);
	}
};
}//end namespace http
}//end namespace network
#endif
