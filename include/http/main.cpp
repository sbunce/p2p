//boost
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

//custom
#include "http.hpp"

//include
#include <CLI_args.hpp>
#include <portable_sleep.hpp>
#include <network.hpp>

//std
#include <map>

boost::mutex Connection_mutex;
std::map<int, boost::shared_ptr<http> > Connection;
std::string web_root;

void connect_call_back(network::socket_data & SD)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.insert(std::make_pair(SD.socket_FD, new http(web_root)));
	SD.recv_call_back = boost::bind(&http::recv_call_back, Connection[SD.socket_FD].get(), _1);
	SD.send_call_back = boost::bind(&http::send_call_back, Connection[SD.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket_data & SD)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(SD.socket_FD);
}

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);

	if(!CLI_Args.get_string("--web_root", web_root)){
		LOGGER << "web root not specified";
		exit(1);
	}

	network Network(&connect_call_back, &disconnect_call_back, 8080);

	/*
	Priveledge drop after starting listener. We try this when the user is
	trying to bind to a port less than 1024 because we assume the program is
	being run as root.

	#ifndef WIN32
	//DEBUG, hardcoded for debian www-data
	if(seteuid(33) < 0){
		fprintf(stderr, "Couldn't set uid.\n");
		exit(1);
	}
	#endif
	*/

	while(true){
		portable_sleep::ms(9999);
	}
}
