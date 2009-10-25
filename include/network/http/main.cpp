//custom
#include "http.hpp"

//include
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <convert.hpp>
#include <CLI_args.hpp>
#include <logger.hpp>
//#include <network/network.hpp>

//standard
#include <csignal>
#include <map>

void connect_call_back(network::connection_info &);
void disconnect_call_back(network::connection_info &);

namespace
{
network::proactor Proactor(
	&connect_call_back,
	&disconnect_call_back,
	"8080"
);

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

void connect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<http> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID, new http(Proactor, web_root)));
	assert(ret.second);
	CI.recv_call_back = boost::bind(&http::recv_call_back, ret.first->second.get(), _1);
}

void disconnect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	Connection.erase(CI.connection_ID);
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

	Proactor.start();

	while(!terminate_program){
/*
		if(Proactor.download_rate() || Proactor.upload_rate()){
			LOGGER << "C<" << Proactor.connections() << "> "
			<< "IC<" << Proactor.incoming_connections() << "> "
			<< "OC<" << Proactor.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Proactor.download_rate()) << "> "
			<< "U<" << convert::size_SI(Proactor.upload_rate()) << ">";
		}
*/
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
}
