//include
#include <atomic_int.hpp>
#include <net/net.hpp>

//standard
#include <csignal>

int fail(0);

boost::mutex test_mutex;
boost::condition_variable_any test_cond;
bool test_finished = false;

void connect_call_back(net::proactor::connection_info &);
void disconnect_call_back(net::proactor::connection_info &);
net::proactor Proactor(
	&connect_call_back,
	&disconnect_call_back
);

const unsigned test_echo(500);
atomic_int<unsigned> echo_count(0);
atomic_int<unsigned> disconnect_count(0);

void recv_call_back(net::proactor::connection_info & CI)
{
	if(CI.direction == net::incoming){
		assert(CI.recv_buf.size() == 1);
		assert(CI.recv_buf[0] == 'x');
		Proactor.send(CI.connection_ID, CI.recv_buf);
		Proactor.disconnect_on_empty(CI.connection_ID);
		++echo_count;
	}else{
		assert(CI.recv_buf.size() == 1);
		assert(CI.recv_buf[0] == 'x');
		Proactor.disconnect(CI.connection_ID);
	}
}

void connect_call_back(net::proactor::connection_info & CI)
{
	CI.recv_call_back = &recv_call_back;
	if(CI.direction == net::outgoing){
		net::buffer buf;
		buf.append('x');
		Proactor.send(CI.connection_ID, buf);
	}
}

void disconnect_call_back(net::proactor::connection_info & CI)
{
	++disconnect_count;
	if(disconnect_count == test_echo * 2){
		boost::mutex::scoped_lock lock(test_mutex);
		test_finished = true;
		test_cond.notify_one();
	}
}

int main()
{
	std::set<net::endpoint> E = net::get_endpoint(
		"127.0.0.1",
		"0"
	);
	assert(!E.empty());
	boost::shared_ptr<net::listener> Listener(new net::listener(*E.begin()));

	if(!Listener->is_open()){
		LOG << "failed to open listener";
		return 1;
	}

	//make sure proactor works after start/stop/start
	Proactor.start(Listener);
	Proactor.stop();
	Proactor.start(Listener);

	for(int x=0; x<test_echo; ++x){
		Proactor.connect("localhost", Proactor.listen_port());
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(test_mutex);
	while(!test_finished){
		test_cond.wait(test_mutex);
	}
	}//end lock scope

	Proactor.stop();

	if(echo_count != test_echo){
		LOG; ++fail;
	}
	if(disconnect_count != test_echo * 2){
		LOG; ++fail;
	}
	return fail;
}
