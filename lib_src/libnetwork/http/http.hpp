#ifndef H_HTTP
#define H_HTTP

//custom
#include "connection.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <network/network.hpp>

class http
{
public:
	http(
		const std::string & web_root_in,
		const std::string & port_in = "0",
		const bool localhost_only_in = false
	);
	~http();

	/* Options
	set_max_upload_rate:
		Set maximum allowed upload rate.
	*/
	void set_max_upload_rate(const unsigned rate);

	/*
	start_0:
		Start the listener.
		Note: Priviledge dropping should be done between steps 0 and 1.
	start:
		Start the HTTP server. Listen on specified listener.
		Note: Listener should not be used after it is passed to this function.
	stop:
		Stop the HTTP server.
	*/
	void start(boost::shared_ptr<network::listener> Listener);
	void stop();

private:
	const std::string web_root;
	const std::string port;
	const bool localhost_only;
	boost::mutex start_stop_mutex;
	network::proactor Proactor;

	//connection specific information
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;

	/*
	connect_call_back:
		Proactor calls back to this when connect happens.
	disconnect_call_back:
		Proactor calls back to this when disconnect happens.
	*/
	void connect_call_back(network::proactor::connection_info & CI);
	void disconnect_call_back(network::proactor::connection_info & CI);
};
#endif
