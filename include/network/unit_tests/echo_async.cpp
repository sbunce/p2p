//include
#include <network/network.hpp>

void connect_call_back(network::connection_info &);
void disconnect_call_back(network::connection_info &);

network::proactor Proactor(
	&connect_call_back,
	&disconnect_call_back
);

void recv_call_back(network::connection_info & CI)
{
	LOGGER << "received " << CI.recv_buf.size();
}

void connect_call_back(network::connection_info & CI)
{
	if(CI.direction == network::incoming){
		LOGGER << "incoming";
		network::buffer buf;
		buf.append('x');
		Proactor.send(CI.connection_ID, buf);
		Proactor.disconnect_on_empty(CI.connection_ID);
	}else{
		LOGGER << "outgoing";
	}
}

void disconnect_call_back(network::connection_info & CI)
{
	LOGGER << CI.IP;
}

int main()
{
	//disable test
	return 0;

	Proactor.start("51500");
	Proactor.connect("localhost", "51500", network::tcp);
	Proactor.stop();
}
