//include
#include <atomic_int.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <convert.hpp>
#include <network/network.hpp>
#include <portable_sleep.hpp>
#include <timer.hpp>

class connection
{
public:
	connection(network::socket_data & Socket)
	{
		if(Socket.direction == network::OUTGOING){
			Socket.send_buff.append('x');
		}
	}

	void recv_call_back(network::socket_data & Socket)
	{
		if(Socket.direction == network::OUTGOING){
			if(Socket.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(Socket.recv_buff[0] != 'y'){
				LOGGER; exit(1);
			}
			Socket.disconnect_flag = true;
		}else{
			if(Socket.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(Socket.recv_buff[0] != 'x'){
				LOGGER; exit(1);
			}
			Socket.send_buff.append('y');
			Socket.recv_buff.clear();
		}
	}

	void send_call_back(network::socket_data & Socket)
	{
		//nothing to do after a send
	}
};

boost::mutex Connection_mutex;
std::map<int, boost::shared_ptr<connection> > Connection;

void connect_call_back(network::socket_data & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::string direction;
	if(Socket.direction == network::INCOMING){
		direction = "in";
	}else{
		direction = "out";
	}
	LOGGER << direction << " H<" << Socket.host << "> IP<" << Socket.IP << "> P<"
		<< Socket.port << "> S<" << Socket.socket_FD << ">";
	Connection.insert(std::make_pair(Socket.socket_FD, new connection(Socket)));
	Socket.recv_call_back = boost::bind(&connection::recv_call_back, Connection[Socket.socket_FD].get(), _1);
	Socket.send_call_back = boost::bind(&connection::send_call_back, Connection[Socket.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket_data & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::string direction;
	if(Socket.direction == network::INCOMING){
		direction = "in";
	}else{
		direction = "out";
	}
	LOGGER << direction << " H<" << Socket.host << "> IP<" << Socket.IP << "> P<"
		<< Socket.port << "> S<" << Socket.socket_FD << ">";
	Connection.erase(Socket.socket_FD);
}

void failed_connect_call_back(const std::string & host, const std::string & port, const network::ERROR error)
{
	std::string reason;
	if(error == network::MAX_CONNECTIONS){
		reason = "max connections";
	}else if(error == network::FAILED_DNS_RESOLUTION){
		reason = "failed DNS resolution";
	}else if(error == network::TIMED_OUT){
		reason = "timed out";
	}else if(error == network::UNKNOWN){
		reason = "unknown";
	}
	LOGGER << "H<" << host << "> P<" << port << ">" << " R<" << reason << ">";
}

int main()
{
	//disable
	//return 0;

	LOGGER << "supported connections: " << network::proactor::max_connections_supported();
	network::proactor Network(
		&failed_connect_call_back,
		&connect_call_back,
		&disconnect_call_back,
		network::proactor::max_connections_supported() / 2,
		network::proactor::max_connections_supported() / 2,
		"6969"
	);

	if(Network.IPv4_enabled()){
		LOGGER << "IPv4 enabled";
	}
	if(Network.IPv6_enabled()){
		LOGGER << "IPv6 enabled";
	}

	if(Network.connect("127.0.0.1", "0")){
		LOGGER << "accepted invalid address";
	}
	if(Network.connect("127.0.0.1", "80000")){
		LOGGER << "accepted invalid address";
	}

	for(int x=0; x<network::proactor::max_connections_supported() / 4; ++x){
		if(Network.IPv4_enabled() && !Network.connect("127.0.0.1", "6969")){
			LOGGER << "rejected valid address";
		}
		if(Network.IPv6_enabled() && !Network.connect("::1", "6969")){
			LOGGER << "rejected valid address";
		}
	}

	bool hack = true; //connections won't immediately be non-zero
	while(Network.connections() != 0 || hack){
		LOGGER << "C<" << Network.connections() << "> "
			<< "IC<" << Network.incoming_connections() << "> "
			<< "OC<" << Network.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Network.current_download_rate()) << "> "
			<< "U<" << convert::size_SI(Network.current_upload_rate()) << ">";
		portable_sleep::ms(1000);
		hack = false;
	}
}
