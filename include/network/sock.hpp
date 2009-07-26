/*
All the reactors deal with sock's. The sock has all data that needs to be
associated with a socket. When the sock is passed to a reactor the reactor
monitors the socket the socket is associated with.

Note: It is not thread safe to modify a sock after it has been passed to a
reactor.
*/
#ifndef H_NETWORK_SOCK
#define H_NETWORK_SOCK

//custom
#include "buffer.hpp"
#include "wrapper.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

//standard
#include <string>

namespace network{
/*
Direction of a connection. An incoming connection is one a remote host
established with us. An outgoing connection is one we establish with a remote
host.
*/
enum DIRECTION {
	INCOMING,
	OUTGOING
};

/*
When a socket is disconnected without the program telling it to an error code
is given.
*/
enum SOCK_ERROR {
	NO_ERROR,        //default, no error
	MAX_CONNECTIONS, //connection limit reached
	TIMEOUT,         //socket timed out
};

/*
This is all the state that needs to be associated with a socket.
*/
class sock
{
public:
	/*
	Constructor for new incoming connection. This is generally used inside a
	reactor when an incoming connection is established.
	*/
	sock(const int socket_FD_in):
		info(new address_info()),
		socket_FD(socket_FD_in),
		IP(wrapper::get_IP(socket_FD)),
		port(wrapper::get_port(socket_FD)),
		last_active(std::time(NULL)),
		direction(INCOMING),
		connect_flag(false),
		disconnect_flag(false),
		failed_connect_flag(false),
		recv_flag(false),
		send_flag(false),
		sock_error(NO_ERROR)
	{
		wrapper::set_non_blocking(socket_FD);
	}

	/*
	Constructor for establishing a new connection. This needs to be created to
	be given to the reactor. Which will in turn connect to the host.
	*/
	sock(boost::shared_ptr<address_info> info_in):
		info(info_in),
		socket_FD(wrapper::socket(*info)),
		host(info->get_host()),
		IP(wrapper::get_IP(*info)),
		port(info->get_port()),
		last_active(std::time(NULL)),
		direction(OUTGOING),
		connect_flag(false),
		disconnect_flag(false),
		failed_connect_flag(false),
		recv_flag(false),
		send_flag(false),
		sock_error(NO_ERROR)
	{
		wrapper::set_non_blocking(socket_FD);
	}

	~sock()
	{
		if(socket_FD != -1){
			close(socket_FD);
		}
	}

	//info for who we're connected to
	boost::shared_ptr<wrapper::address_info> info;

	const int socket_FD;       //file descriptor for network connection
	const std::string host;    //name we connected to (ie "google.com")
	const std::string IP;      //IP host resolved to
	const std::string port;    //if listen_port == port then connection is incoming
	const DIRECTION direction; //INCOMING or OUTGOING

	/*
	connect_flag:
		When false a connect job needs to be done. When true a connect job
		has already been done. The reactor sets this to true. The const is like
		a safety cover on this.
	disconnect_flag:
		if this is set to true the socket will be disconnected.
	failed_connect_flag:
		If true the failed connect call back needs to be done. No other call
		backs should be done after the failed connect call back.
	recv_flag:
		If true the recv call back needs to be done.
	send_flag:
		If true the send call back needs to be done.
	*/
	const bool connect_flag;
	bool disconnect_flag;
	bool failed_connect_flag;
	bool recv_flag;
	bool send_flag;

	//send()/recv() buffers
	buffer recv_buff;
	buffer send_buff;

	//call backs used by the proactor
	boost::function<void (sock &)> recv_call_back;
	boost::function<void (sock &)> send_call_back;

	//error stored here after abnormal disconnect
	SOCK_ERROR sock_error;

	/* Used by Reactor.
	seen:
		Updates last seen. Used for timeouts. Used by reactor but harmless to call
		from outside the reactor.
	last_seen:
		Returns how many seconds since the socket was last has activity.
	*/
	void seen(){ last_active = std::time(NULL); }
	std::time_t last_seen() { return std::time(NULL) - last_active; }

private:
	//last time seen (used for timeouts)
	std::time_t last_active;
};
}//end of network namespace
#endif
