//boost
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

//include
#include <network.hpp>

class connection
{
public:
	connection(
		const bool server_in,
		network::socket & Socket
	):
		server(server_in)
	{
		if(!server){
			//client starts by sending a request
			Socket.send_buff.append('x');
		}
	}

	void recv_call_back(network::socket & Socket)
	{
		if(server){
			if(Socket.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(Socket.recv_buff[0] != 'x'){
				LOGGER; exit(1);
			}
			Socket.send_buff.append('y');
			Socket.recv_buff.clear();
		}else{
			if(Socket.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(Socket.recv_buff[0] != 'y'){
				LOGGER; exit(1);
			}
			Socket.disconnect_flag = true;
		}
	}

	void send_call_back(network::socket & Socket)
	{

	}

	//true if this is server
	bool server;
};

boost::mutex Connection_mutex;
std::map<int, boost::shared_ptr<connection> > Connection;

void connect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	if(Socket.outgoing){
		LOGGER << "outgoing connect socket:" << Socket.socket_FD << " IP:" << Socket.IP
			<< " port:" << Socket.port;
		Connection.insert(std::make_pair(Socket.socket_FD, new connection(false, Socket)));
	}else{
		LOGGER << "incoming connect socket:" << Socket.socket_FD << " IP:" << Socket.IP
			<< " port:" << Socket.port;
		Connection.insert(std::make_pair(Socket.socket_FD, new connection(true, Socket)));
	}
	Socket.recv_call_back = boost::bind(&connection::recv_call_back, Connection[Socket.socket_FD].get(), _1);
	Socket.send_call_back = boost::bind(&connection::send_call_back, Connection[Socket.socket_FD].get(), _1);
}

void disconnect_call_back(network::socket & Socket)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOGGER << "disconnect socket:" << Socket.socket_FD << " IP:" << Socket.IP
		<< " port:" << Socket.port;
	Connection.erase(Socket.socket_FD);
}

void failed_connect_call_back(network::socket & Socket)
{
	LOGGER << "failed connect: " << Socket.IP << ":" << Socket.port;
	LOGGER; exit(1);
}

int main()
{
	return 0;
	network Network(&failed_connect_call_back, &connect_call_back, &disconnect_call_back, 6969);

	for(int x=0; x<500; ++x){
		if(!Network.add_connection("127.0.0.1", 6969)){
			LOGGER; exit(1);
		}
	}

	sleep(10);
}
