//custom
#include "http.hpp"

//include
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <convert.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
#include <network/network.hpp>

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
	assert(S.error != network::sock::no_error);
	std::string reason;
	if(S.error == network::sock::failed_resolve){
		reason = "failed resolve";
	}else if(S.error == network::sock::max_connections){
		reason = "max connections";
	}else if(S.error == network::sock::timed_out){
		reason = "timed_out";
	}else if(S.error == network::sock::other_error){
		reason = "other";
	}else{
		LOGGER << "unrecognized failure reason";
		exit(1);
	}
	LOGGER << "H<" << S.host << "> P<" << S.port << ">" << " R<" << reason << ">";
	exit(1);
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
		&connect_call_back,
		&disconnect_call_back,
		&failed_connect_call_back,
		true,
		"8080"
	);
	Proactor.max_connections(Proactor.supported_connections(), 0);
	Proactor.start();

	while(!terminate_program){
		if(Proactor.download_rate() || Proactor.upload_rate()){
			LOGGER << "C<" << Proactor.connections() << "> "
			<< "IC<" << Proactor.incoming_connections() << "> "
			<< "OC<" << Proactor.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Proactor.download_rate()) << "> "
			<< "U<" << convert::size_SI(Proactor.upload_rate()) << ">";
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
}
