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
	Connection.insert(std::make_pair(S.socket_FD, new http(web_root)));
	S.recv_call_back = boost::bind(&http::recv_call_back, Connection[S.socket_FD].get(), _1);
	S.send_call_back = boost::bind(&http::send_call_back, Connection[S.socket_FD].get(), _1);
}

void disconnect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(S.socket_FD);
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

	network::reactor_select Reactor("8080");
	network::proactor Proactor(
		Reactor,
		&connect_call_back,
		&disconnect_call_back,
		&failed_connect_call_back
	);
	Reactor.set_max_upload_rate(1024*500);
	Reactor.start();
	Proactor.start();

	while(!terminate_program){
		if(Reactor.current_download_rate() || Reactor.current_upload_rate()){
			LOGGER << "C<" << Reactor.connections() << "> "
			<< "IC<" << Reactor.incoming_connections() << "> "
			<< "OC<" << Reactor.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Reactor.current_download_rate()) << "> "
			<< "U<" << convert::size_SI(Reactor.current_upload_rate()) << ">";
		}
		portable_sleep::ms(1000);
	}
	Reactor.stop();
	Proactor.stop();

	LOGGER << "jobs: " << Reactor.job_count;
}
