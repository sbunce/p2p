//include
#include <network/network.hpp>

int main()
{
	//setup listener
	std::set<network::endpoint> E = network::get_endpoint("localhost", "0", network::tcp);
	assert(!E.empty());
	network::listener L(*E.begin());
	assert(L.is_open());

	//connect
	E = network::get_endpoint("localhost", L.port(), network::tcp);
	assert(!E.empty());
	network::nstream N_client(*E.begin());
	assert(N_client.is_open());

	//accept connection on listener
	boost::shared_ptr<network::nstream> N_server = L.accept();
	assert(N_server);

	//send test message client -> server
	network::buffer B("123ABC");
	N_client.send(B);
	assert(B.empty());

	//echo back test message server -> client
	N_server->recv(B);
	assert(B.str() == "123ABC");
	N_server->send(B);
	assert(B.empty());

	//recv echo'd test message on client
	N_client.recv(B);
	assert(B.str() == "123ABC");
}
