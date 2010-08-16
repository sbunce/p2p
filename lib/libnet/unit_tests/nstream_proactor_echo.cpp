//include
#include <net/net.hpp>
#include <unit_test.hpp>

//standard
#include <csignal>

int fail(0);
boost::shared_ptr<net::nstream_proactor> Proactor;

void connect_call_back(net::nstream_proactor::connect_event CE)
{

}

void disconnect_call_back(net::nstream_proactor::disconnect_event DE)
{

}

void recv_call_back(net::nstream_proactor::recv_event RE)
{

}

void send_call_back(net::nstream_proactor::send_event SE)
{

}

int main()
{
	unit_test::timeout();
	Proactor.reset(new net::nstream_proactor(
		&connect_call_back,
		&disconnect_call_back,
		&recv_call_back,
		&send_call_back
	));
	std::set<net::endpoint> E = net::get_endpoint("127.0.0.1", "8080");
	assert(!E.empty());
	Proactor->listen(*E.begin());


	//sleep(8);
/*
	E = net::get_endpoint("localhost", Listener->port());
	assert(!E.empty());
*/

	return fail;
}
