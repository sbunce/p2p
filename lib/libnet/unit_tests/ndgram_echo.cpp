//include
#include <net/net.hpp>
#include <unit_test.hpp>

int main()
{
	unit_test::timeout();

	//setup echo client/server
	std::set<net::endpoint> E = net::get_endpoint("localhost", "0");
	assert(!E.empty());
	net::ndgram N_serv(*E.begin());
	assert(N_serv.is_open());
	net::ndgram N_client(*E.begin());
	assert(N_client.is_open());

	//send character from client to server
	assert(N_serv.local_ep());
	E = net::get_endpoint("localhost", N_serv.local_ep()->port());
	assert(!E.empty());
	net::buffer buf("x");
	N_client.send(buf, *E.begin());
	assert(buf.empty());

	//recv character on server and echo it back to client
	boost::optional<net::endpoint> from;
	N_serv.recv(buf, from);
	assert(buf.str() == "x");
	assert(from);
	N_serv.send(buf, *from);

	//recv character on client
	buf.clear();
	N_client.recv(buf, from);
	assert(buf.str() == "x");
	assert(from);
}
