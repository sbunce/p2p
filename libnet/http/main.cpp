//boost
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

//custom
#include "http.hpp"

//include
#include <CLI_args.hpp>
#include <logger.hpp>
#include <portable_sleep.hpp>
#include <network.hpp>

//std
#include <csignal>
#include <map>

const std::string listen_port = "8080";

namespace
{
	//connection specific information
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<http> > Connection;

	//path to web root
	std::string web_root;

	//volatile needed to stop optimizer from check-once behavior on while(!quit)
	volatile sig_atomic_t terminate_program = 0;

	void signal_handler(int sig)
	{
		signal(sig, signal_handler);
		terminate_program = 1;
	}
}//end of unnamed namespace

void connect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.insert(std::make_pair(Socket.socket_FD, new http(web_root)));
	Socket.recv_call_back = boost::bind(&http::recv_call_back, Connection[Socket.socket_FD].get(), _1);
	Socket.send_call_back = boost::bind(&http::send_call_back, Connection[Socket.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(Socket.socket_FD);
}

void failed_connect_call_back(network::socket & Socket)
{
	LOGGER << "failed connect: " << Socket.IP << ":" << Socket.port;
}

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);
	if(!CLI_Args.get_string("--web_root", web_root)){
		LOGGER << "web root not specified";
		exit(1);
	}

	network::io_service Network(
		&failed_connect_call_back,
		&connect_call_back,
		&disconnect_call_back,
		"8080"
	);
	Network.set_max_download_rate(25*1024);

	//register signal handlers
	signal(SIGINT, signal_handler);

	while(!terminate_program){
		portable_sleep::ms(1000);
	}
}
