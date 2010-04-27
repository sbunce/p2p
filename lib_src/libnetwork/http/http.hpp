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
	http(const std::string & web_root_in);
	~http();

	/*
	port:
		Return port server is listening on.
	set_max_upload_rate:
		Set maximum allowed upload rate.
	*/
	std::string port();
	void set_max_upload_rate(const unsigned rate);

	/*
	start:
		Start the HTTP server.
	*/
	void start(const std::string & port = "0", const bool localhost_only = false);

private:
	network::proactor Proactor;
	std::string web_root;

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
