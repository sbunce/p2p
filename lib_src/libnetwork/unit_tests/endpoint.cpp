//include
#include <logger.hpp>
#include <network/network.hpp>

int fail(0);

void test(const std::string & addr, const std::string & port)
{
	LOGGER << "testing: " << addr << " " << port;

}

int main()
{
	network::start();

	{//BEGIN IPv4
	std::set<network::endpoint> E = network::get_endpoint("127.0.0.1", "0", network::tcp);
	assert(!E.empty());

	//copy endpoint with binary functions
	network::endpoint test_1 = *E.begin();
	std::string IP_bin = test_1.IP_bin();
	std::string port_bin = test_1.port_bin();
	network::endpoint test_2(IP_bin, port_bin, network::tcp);

	//check if endpoints are the same
	if(test_1 != test_2){
		LOGGER; ++fail;
	}

	if(test_1.version() != network::IPv4){
		LOGGER; ++fail;
	}
	}//END IPv4

	{//BEGIN IPv6
	std::set<network::endpoint> E = network::get_endpoint("::1", "0", network::tcp);
	assert(!E.empty());

	//copy endpoint with binary functions
	network::endpoint test_1 = *E.begin();
	std::string IP_bin = test_1.IP_bin();
	std::string port_bin = test_1.port_bin();
	network::endpoint test_2(IP_bin, port_bin, network::tcp);

	//check if endpoints are the same
	if(test_1 != test_2){
		LOGGER; ++fail;
	}

	if(test_1.version() != network::IPv6){
		LOGGER; ++fail;
	}
	}//END IPv6

	return fail;
}
