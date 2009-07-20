//custom
#include "../network.hpp"
#include "http.hpp"

//include
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <convert.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
#include <portable_sleep.hpp>

//standard
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

void connect_call_back(network::socket_data & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.insert(std::make_pair(Socket.socket_FD, new http(web_root)));
	Socket.recv_call_back = boost::bind(&http::recv_call_back, Connection[Socket.socket_FD].get(), _1);
	Socket.send_call_back = boost::bind(&http::send_call_back, Connection[Socket.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket_data & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(Socket.socket_FD);
}

void failed_connect_call_back(const std::string & host, const std::string & port, const network::ERROR error)
{
	std::string reason;
	if(error == network::MAX_CONNECTIONS){
		reason = "max connections";
	}else if(error == network::FAILED_DNS_RESOLUTION){
		reason = "failed DNS resolution";
	}else if(error == network::TIMED_OUT){
		reason = "timed out";
	}else if(error == network::UNKNOWN){
		reason = "unknown";
	}
	LOGGER << "H<" << host << "> P<" << port << ">" << " R<" << reason << ">";
}

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);
	if(!CLI_Args.get_string("--web_root", web_root)){
		LOGGER << "web root not specified";
		exit(1);
	}

	LOGGER << "supported connections: " << network::proactor::max_connections_supported();
	network::proactor Network(
		&failed_connect_call_back,
		&connect_call_back,
		&disconnect_call_back,
		network::proactor::max_connections_supported(),
		0,
		"8080"
	);
	//Network.set_max_upload_rate(1024*1024);

	//register signal handlers
	signal(SIGINT, signal_handler);

	while(!terminate_program){
		if(Network.connections() != 0 || Network.current_download_rate()
			|| Network.current_upload_rate())
		{
			LOGGER << "C<" << Network.connections() << "> "
			<< "IC<" << Network.incoming_connections() << "> "
			<< "OC<" << Network.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Network.current_download_rate()) << "> "
			<< "U<" << convert::size_SI(Network.current_upload_rate()) << ">";
		}
		portable_sleep::ms(1000);
	}
}
