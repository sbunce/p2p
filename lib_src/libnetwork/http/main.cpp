//custom
#include "http.hpp"

//include
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <convert.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
#include <network/network.hpp>
#include <portable_sleep.hpp>

//standard
#include <csignal>
#include <map>

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

void connect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<http> >::iterator, bool>
		ret = Connection.insert(std::make_pair(S.socket_FD, new http(web_root)));
	assert(ret.second);
	S.recv_call_back = boost::bind(&http::recv_call_back, ret.first->second.get(), _1);
	S.send_call_back = boost::bind(&http::send_call_back, ret.first->second.get(), _1);
}

void disconnect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	if(Connection.erase(S.socket_FD) != 1){
		LOGGER << "disconnect called for socket that never connected";
		exit(1);
	}
}

void failed_connect_call_back(network::sock & S)
{
	LOGGER << "H<" << S.host << "> P<" << S.port << ">";
}

int main(int argc, char ** argv)
{
	//register signal handlers
	signal(SIGINT, signal_handler);

	CLI_args CLI_Args(argc, argv);
	if(!CLI_Args.get_string("--web_root", web_root)){
		LOGGER << "web root not specified";
		exit(1);
	}

	network::proactor Proactor(
		"8080",
		&connect_call_back,
		&disconnect_call_back,
		&failed_connect_call_back
	);
	//Proactor.Reactor.max_upload_rate(1024*500);
	Proactor.max_connections(Proactor.max_connections_supported(), 0);

	while(!terminate_program){
		if(Proactor.current_download_rate() || Proactor.current_upload_rate()){
			LOGGER << "C<" << Proactor.connections() << "> "
			<< "IC<" << Proactor.incoming_connections() << "> "
			<< "OC<" << Proactor.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Proactor.current_download_rate()) << "> "
			<< "U<" << convert::size_SI(Proactor.current_upload_rate()) << ">";
		}
		portable_sleep::ms(1000);
	}
}
