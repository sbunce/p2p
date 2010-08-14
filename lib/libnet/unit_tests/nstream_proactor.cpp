//include
#include <atomic_int.hpp>
#include <net/net.hpp>
#include <unit_test.hpp>

//standard
#include <csignal>

int fail(0);

int main()
{
	unit_test::timeout();
/*
	std::set<net::endpoint> E = net::get_endpoint("127.0.0.1", "0");
	assert(!E.empty());
	boost::shared_ptr<net::listener> L(new net::listener(*E.begin()));
	net::nstream_reactor NR;
	NR.start(L);
	E = net::get_endpoint("127.0.0.1", L->port());


	NR.stop();
*/
	return fail;
}
