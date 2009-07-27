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
	FAILED_RESOLVE,  //failed to resolve host
	MAX_CONNECTIONS, //connection limit reached
	TIMEOUT,         //socket timed out
	OTHER            //error there is no other enum for
};

/*
This is all the state that needs to be associated with a socket.
*/
class sock
{
	static const int default_timeout = 16;
public:
	/*
	Constructor for new incoming connection. This is generally used inside a
	reactor when an incoming connection is established.
	*/
	sock(
		const int socket_FD_in
	):
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
		sock_error(NO_ERROR),
		timeout(default_timeout)
	{}

	/*
	Constructor for establishing a new connection. This needs to be created to
	be given to the reactor. Which will in turn connect to the host.
	*/
	sock(
		boost::shared_ptr<address_info> info_in
	):
		info(info_in),
		socket_FD(-1),
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
		sock_error(NO_ERROR),
		timeout(default_timeout)
	{}

	~sock()
	{
		if(socket_FD != -1){
			close(socket_FD);
		}
	}

	//info for who we're connected to
	boost::shared_ptr<wrapper::address_info> info;

	/*
	File descriptor for network connection. If the second ctor is used this is
	set to -1. The reactor will const_cast this and set it when connecting. The
	const acts as a safety cover here.
	*/
	const int socket_FD;

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

	/*
	If the socket is in the reactor for this long it will time out. This may be
	changed from the default value. Value is in seconds.
	*/
	std::time_t timeout;

	/*
	Updates last_active. Used for timeouts. Used by reactor but harmless to call
	from outside the reactor (although this has no meaning).
	*/
	void seen()
	{
		last_active = std::time(NULL);
	}

	//returns true if the socket has timed out
	bool timed_out()
	{
		return std::time(NULL) - last_active > timeout;
	}

private:
	//last time seen (used for timeouts)
	std::time_t last_active;
};
}//end of network namespace
#endif
