//include
#include <network/network.hpp>

void connect_call_back(network::connection_info &);
void recv_call_back(network::connection_info &);
void send_call_back(network::connection_info &);
void disconnect_call_back(network::connection_info &);

network::proactor P(
	&connect_call_back,
	&disconnect_call_back
);

void connect_call_back(network::connection_info & CI)
{
	LOGGER << "connected to " << CI.IP << " " << CI.port;
	CI.recv_call_back = &recv_call_back;
	CI.send_call_back = &send_call_back;

	network::buffer B(
		"GET / HTTP/1.1\r\n"
		"Connection: close\r\n\r\n"
	);

	P.write(CI.connection_ID, B);	
}

void recv_call_back(network::connection_info & CI)
{
	LOGGER << "read size: " << CI.recv_buf.size();
}

void send_call_back(network::connection_info & CI)
{
	LOGGER << "send buf size: " << CI.send_buf_size;
}

void disconnect_call_back(network::connection_info & CI)
{
	LOGGER << "disconnect ";
}

int main()
{
	//disable test
	return 0;

	P.start();
	P.connect("seth.dnsdojo.net", "80", network::tcp);
	boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
}
