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

	*/

	/* Info
	port:
		Return port server is listening on.
	set_max_upload_rate:
		Set maximum allowed upload rate.
	*/
	std::string listen_port();
	void set_max_upload_rate(const unsigned rate);

	/*
	start_0:
		Start the listener.
		Note: Priviledge dropping should be done between steps 0 and 1.
	start_1:
		Start the rest of the server. Allow incoming connections.
	stop:
		Stop the HTTP server.
	*/
	void start_0();
	void start_1();
	void stop();

private:
	const std::string web_root;
	const std::string port;
	const bool localhost_only;

	//lock for start()/stop()
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
	void connect_call_back(network::connection_info & CI);
	void disconnect_call_back(network::connection_info & CI);
};
#endif
