//include
#include <network/network.hpp>

int main()
{
	network::start();

	//setup echo server
	std::set<network::endpoint> E = network::get_endpoint("", "0", network::udp);
	assert(!E.empty());
	network::ndgram N_serv(*E.begin());
	assert(N_serv.is_open());
	LOGGER << "local_IP: " << N_serv.local_IP() << " local_port: " << N_serv.local_port();

	//setup echo client
	E = network::get_endpoint("localhost", N_serv.local_port(), network::udp);
	assert(!E.empty());
	network::ndgram N_client;
	network::buffer buf("x");
	//N_client.send();
}
