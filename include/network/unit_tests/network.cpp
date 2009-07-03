//include
#include <atomic_int.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <convert.hpp>
#include <network/network.hpp>
#include <portable_sleep.hpp>
#include <timer.hpp>

const std::string listen_port = "6969";

class connection
{
public:
	connection(network::socket & Socket)
	{
		if(Socket.direction == network::OUTGOING){
			Socket.send_buff.append('x');
		}
	}

	void recv_call_back(network::socket & Socket)
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

	void send_call_back(network::socket & Socket)
	{
		//nothing to do after a send
	}
};

boost::mutex Connection_mutex;
std::map<int, boost::shared_ptr<connection> > Connection;

void connect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::string direction;
	if(Socket.direction == network::INCOMING){
		direction = "in";
	}else{
		direction = "out";
	}
	//LOGGER << direction << " H<" << Socket.host << "> IP<" << Socket.IP << "> P<"
	//	<< Socket.port << "> S<" << Socket.socket_FD << ">";
	std::cout << "+";
	Connection.insert(std::make_pair(Socket.socket_FD, new connection(Socket)));
	Socket.recv_call_back = boost::bind(&connection::recv_call_back, Connection[Socket.socket_FD].get(), _1);
	Socket.send_call_back = boost::bind(&connection::send_call_back, Connection[Socket.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::string direction;
	if(Socket.direction == network::INCOMING){
		direction = "in";
	}else{
		direction = "out";
	}
	//LOGGER << direction << " H<" << Socket.host << "> IP<" << Socket.IP << "> P<"
	//	<< Socket.port << "> S<" << Socket.socket_FD << ">";
	std::cout << "-";
	Connection.erase(Socket.socket_FD);
}

void failed_connect_call_back(network::socket & Socket)
{
	std::string reason;
	if(Socket.error == network::MAX_CONNECTIONS){
		reason = "max connections";
	}else if(Socket.error == network::FAILED_VALID){
		reason = "failed valid";
	}else if(Socket.error == network::FAILED_INVALID){
		reason = "failed invalid";
	}else{
		reason = "unknown";
	}
	LOGGER << "H<" << Socket.host << "> P<" << Socket.port << ">" << " R<" << reason << ">";
}

int main()
{
	LOGGER << "supported connections: " << network::io_service::max_connections_supported();
	network::io_service Network(
		&failed_connect_call_back,
		&connect_call_back,
		&disconnect_call_back,
		network::io_service::max_connections_supported() / 2,
		network::io_service::max_connections_supported() / 2,
		"6969"
	);

	if(Network.connect("127.0.0.1", "0")){
		LOGGER << "accepted invalid address";
	}
	if(Network.connect("127.0.0.1", "80000")){
		LOGGER << "accepted invalid address";
	}

	boost::uint64_t start = timer::TSC();
	for(int x=0; x<network::io_service::max_connections_supported() / 4; ++x){
		if(!Network.connect("127.0.0.1", "6969")){
			LOGGER << "rejected valid address";
		}
		if(!Network.connect("::1", "6969")){
			LOGGER << "rejected valid address";
		}
	}

	bool hack = true;
	while(Network.connections() != 0 || hack){
		LOGGER << "C<" << Network.connections() << "> "
			<< "IC<" << Network.incoming_connections() << "> "
			<< "OC<" << Network.outgoing_connections() << "> "
			<< "D<" << convert::size_SI(Network.current_download_rate()) << "> "
			<< "U<" << convert::size_SI(Network.current_upload_rate()) << ">";
		portable_sleep::ms(1000);
		hack = false;
	}
	boost::uint64_t end = timer::TSC();
	LOGGER << network::io_service::max_connections_supported() /
		timer::elapsed(start, end, timer::SECONDS) << " CPS";
}
