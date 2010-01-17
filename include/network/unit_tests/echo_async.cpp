//include
#include <atomic_int.hpp>
#include <network/network.hpp>

//standard
#include <csignal>

boost::mutex terminate_mutex;
boost::condition_variable_any terminate_cond;

volatile sig_atomic_t terminate_program = 0;

void signal_handler(int sig)
{
	signal(sig, signal_handler);
	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	terminate_program = 1;
	terminate_cond.notify_one();
	}//end lock scope
}

void connect_call_back(network::connection_info &);
void disconnect_call_back(network::connection_info &);
network::proactor Proactor(
	&connect_call_back,
	&disconnect_call_back
);

const unsigned test_echo(8);
atomic_int<unsigned> echo_count(0);
atomic_int<unsigned> disconnect_count(0);

void recv_call_back(network::connection_info & CI)
{
	if(CI.direction == network::incoming){
		assert(CI.recv_buf.size() == 1);
		Proactor.send(CI.connection_ID, CI.recv_buf);
		Proactor.disconnect_on_empty(CI.connection_ID);
	}else{
		assert(CI.recv_buf.size() == 1);
		Proactor.disconnect(CI.connection_ID);
	}
	++echo_count;
	if(echo_count == test_echo){
		boost::mutex::scoped_lock lock(terminate_mutex);
		terminate_program = 1;
		terminate_cond.notify_one();
	}
}

void connect_call_back(network::connection_info & CI)
{
	CI.recv_call_back = &recv_call_back;
	if(CI.direction == network::outgoing){
		network::buffer buf;
		buf.append('x');
		Proactor.send(CI.connection_ID, buf);
	}
}

void disconnect_call_back(network::connection_info & CI)
{
	++disconnect_count;
}

int main()
{
	std::set<network::endpoint> E = network::get_endpoint(
		"localhost",
		"0",
		network::tcp
	);
	assert(!E.empty());
	Proactor.start(*E.begin());

	for(int x=0; x<test_echo; ++x){
		Proactor.connect("localhost", Proactor.listen_port(), network::tcp);
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(terminate_mutex);
	while(!terminate_program){
		terminate_cond.wait(terminate_mutex);
	}
	}//end lock scope

	Proactor.stop();

	if(disconnect_count != test_echo * 2){
		LOGGER << disconnect_count; exit(1);
	}
}
