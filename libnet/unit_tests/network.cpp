//boost
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

//include
#include <portable_sleep.hpp>

//network
#include <network.hpp>

const std::string listen_port = "6969";

class connection
{
public:
	connection(network::socket & Socket)
	{
		if(Socket.port != listen_port){
			//client starts by sending a request
			Socket.send_buff.append('x');
		}
	}

	void recv_call_back(network::socket & Socket)
	{
		if(Socket.port != listen_port){
			//client
			if(Socket.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(Socket.recv_buff[0] != 'y'){
				LOGGER; exit(1);
			}
			Socket.disconnect_flag = true;
		}else{
			//server
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
	if(Socket.port != listen_port){
		LOGGER << "inbound " << Socket.host << "/" << Socket.IP << "/" << Socket.port << "/" << Socket.socket_FD;
	}else{
		LOGGER << "outbound " << Socket.host << "/" << Socket.IP << "/" << Socket.port << "/" << Socket.socket_FD;
	}
	Connection.insert(std::make_pair(Socket.socket_FD, new connection(Socket)));
	Socket.recv_call_back = boost::bind(&connection::recv_call_back, Connection[Socket.socket_FD].get(), _1);
	Socket.send_call_back = boost::bind(&connection::send_call_back, Connection[Socket.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOGGER << "disconnect " << Socket.host << "/" << Socket.IP << "/" << Socket.port << "/" << Socket.socket_FD;
	Connection.erase(Socket.socket_FD);
}

void failed_connect_call_back(network::socket & Socket)
{
	LOGGER << "connect failed " << Socket.host << "/" << Socket.IP << "/" << Socket.port;
}

int main()
{
	network::io_service Network(
		&failed_connect_call_back,
		&connect_call_back,
		&disconnect_call_back
	);

	Network.connect("123.123.123.123", "6969");

	portable_sleep::ms(5*1000);
}
