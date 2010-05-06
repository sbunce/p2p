//include
#include <net/net.hpp>

int main()
{
	//setup echo server
	std::set<net::endpoint> E = net::get_endpoint("localhost", "0");
	assert(!E.empty());
	net::ndgram N_serv(*E.begin());
	assert(N_serv.is_open());
	LOG << "local_IP: " << N_serv.local_IP() << " local_port: " << N_serv.local_port();

	//setup echo client
	E = net::get_endpoint("localhost", N_serv.local_port());
	assert(!E.empty());
	net::ndgram N_client;
	assert(N_client.is_open());

	//send character to server
	net::buffer buf("x");
	N_client.send(buf, *E.begin());
	assert(buf.empty());

	//recv character and echo it back
	boost::shared_ptr<net::endpoint> from;
	N_serv.recv(buf, from);
	assert(buf.str() == "x");
	N_serv.send(buf, *from);

	//recv character on client
	buf.clear();
	N_client.recv(buf, from);
	assert(buf.str() == "x");
}
