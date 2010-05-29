//include
#include <net/net.hpp>
#include <unit_test.hpp>

int main()
{
	unit_test::timeout();

	//setup listener
	std::set<net::endpoint> E = net::get_endpoint("localhost", "0");
	assert(!E.empty());
	net::listener L(*E.begin());
	assert(L.is_open());

	//connect
	E = net::get_endpoint("localhost", L.port());
	assert(!E.empty());
	net::nstream N_client(*E.begin());
	assert(N_client.is_open());

	//accept connection on listener
	boost::shared_ptr<net::nstream> N_server = L.accept();
	assert(N_server);

	//send test message client -> server
	net::buffer B("ABC");
	N_client.send(B);
	assert(B.empty());

	//echo back test message server -> client
	N_server->recv(B);
	assert(B.str() == "ABC");
	N_server->send(B);
	assert(B.empty());

	//recv echo'd test message on client
	N_client.recv(B);
	assert(B.str() == "ABC");
}
