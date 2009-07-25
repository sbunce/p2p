//include
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <convert.hpp>
#include <network/network.hpp>
#include <portable_sleep.hpp>

class connection
{
public:
	connection(network::sock & S)
	{
		if(S.direction == network::OUTGOING){
			S.send_buff.append('x');
		}
	}

	void recv_call_back(network::sock & S)
	{
		if(S.direction == network::OUTGOING){
			if(S.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(S.recv_buff[0] != 'y'){
				LOGGER; exit(1);
			}
			S.disconnect_flag = true;
		}else{
			if(S.recv_buff.size() != 1){
				LOGGER; exit(1);
			}
			if(S.recv_buff[0] != 'x'){
				LOGGER; exit(1);
			}
			S.send_buff.append('y');
			S.recv_buff.clear();
		}
	}

	void send_call_back(network::sock & S)
	{
		//nothing to do after a send
	}
};

boost::mutex Connection_mutex;
std::map<int, boost::shared_ptr<connection> > Connection;

void connect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::string direction;
	if(S.direction == network::INCOMING){
		direction = "in ";
	}else{
		direction = "out";
	}
	LOGGER << direction << " H<" << S.host << "> IP<" << S.IP << "> P<"
		<< S.port << "> S<" << S.socket_FD << ">";
	Connection.insert(std::make_pair(S.socket_FD, new connection(S)));
	S.recv_call_back = boost::bind(&connection::recv_call_back, Connection[S.socket_FD].get(), _1);
	S.send_call_back = boost::bind(&connection::send_call_back, Connection[S.socket_FD].get(), _1);
}

void disconnect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::string direction;
	if(S.direction == network::INCOMING){
		direction = "in";
	}else{
		direction = "out";
	}
	LOGGER << direction << " H<" << S.host << "> IP<" << S.IP << "> P<"
		<< S.port << "> S<" << S.socket_FD << ">";
	Connection.erase(S.socket_FD);
}

void failed_connect_call_back(network::sock & S)
{
	LOGGER << "H<" << S.host << "> P<" << S.port << ">";
}

int main()
{
	//disable
	//return 0;

	network::reactor_select Reactor("65001");
	network::proactor Proactor(
		Reactor,
		&connect_call_back,
		&disconnect_call_back,
		&failed_connect_call_back
	);
	Proactor.start();

	for(int x=0; x<16; ++x){
		boost::shared_ptr<network::address_info> AI(
			new network::address_info("127.0.0.1", "65001"));
		boost::shared_ptr<network::sock> S(new network::sock(AI));
		Reactor.connect(S);
	}

/*
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
*/
	portable_sleep::ms(10*1000);

	Proactor.stop();
}
