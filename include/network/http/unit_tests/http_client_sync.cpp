//include
#include <network/network.hpp>

int main()
{
	//http server to connect to and test
	network::http::server HTTP(".", "0", true);

	//connect
	std::set<network::endpoint> E = network::get_endpoint("localhost", HTTP.port(), network::tcp);
	if(E.empty()){
		LOGGER; exit(1);
	}
	network::nstream N(*E.begin());
	if(!N.is_open()){
		LOGGER; exit(1);
	}

	//send request
	network::buffer B("GET /\r\n\r\n");
	while(!B.empty() && N.send(B) > 0);
	if(!N.is_open()){
		LOGGER; exit(1);
	}

	//recv response
	while(N.recv(B) > 0);
}
