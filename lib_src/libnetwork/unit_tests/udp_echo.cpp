//include
#include <network/network.hpp>

int main()
{
	network::start();

	//setup echo server
	std::set<network::endpoint> E = network::get_endpoint("localhost", "0", network::udp);
	assert(!E.empty());
	network::ndgram N_serv(*E.begin());
	assert(N_serv.is_open());
	LOGGER << "local_IP: " << N_serv.local_IP() << " local_port: " << N_serv.local_port();

	//setup echo client
	E = network::get_endpoint("localhost", N_serv.local_port(), network::udp);
	assert(!E.empty());
	network::ndgram N_client;
	assert(N_client.is_open());

	//send character to server
	network::buffer buf("x");
	N_client.send(buf, *E.begin());
	assert(buf.empty());

	//recv character and echo it back
	boost::shared_ptr<network::endpoint> from;
	N_serv.recv(buf, from);
	assert(buf.str() == "x");
	N_serv.send(buf, *from);

	//recv character on client
	buf.clear();
	N_client.recv(buf, from);
	assert(buf.str() == "x");
}
