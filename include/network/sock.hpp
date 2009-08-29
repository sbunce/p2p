/*
All the reactors deal with sock's. The sock has all data that needs to be
associated with a socket. When the sock is passed to a reactor the reactor
monitors the socket the sock is associated with.

Note: It is not thread safe to modify a sock after it has been passed to a
reactor.
	Passing the sock to or from the reactor is transfering ownership of
what thread gets to modify the sock. When the sock is given to the proactor
(by returning it from call_back_get_job) the reactor cannot modify it until the
proactor passes it back. Likewise when the proactor passes the sock back to the
reactor the proactor cannot modify the sock until it is returned from the
reactor. The exception to this is the const members of the sock which may be
read at any time by any thread.
*/
#ifndef H_NETWORK_SOCK
#define H_NETWORK_SOCK

//include
#include <atomic_bool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <network/network.hpp>

//standard
#include <string>

namespace network {

//used for PIMPL
namespace wrapper {
class address_info;
}
typedef wrapper::address_info address_info;

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

class sock
{
	static const int DEFAULT_TIMEOUT = 16;
public:
	/*
	Constructor for new incoming connection. This is generally used inside a
	reactor when an incoming connection is established.
	*/
	sock(const int socket_FD_in);

	/*
	Constructor for establishing a new connection. This needs to be created to
	be given to the reactor. Which will in turn connect to the host.
	*/
	sock(boost::shared_ptr<address_info> info_in);

	~sock();

	//info for who we're connected to
	boost::shared_ptr<address_info> info;

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

	/* Flags to indicate what has happended to the sock in the reactor.
	connect_flag:
		When false a connect job needs to be done. When true a connect job
		has already been done. The reactor sets this to true.
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
	bool connect_flag;
	bool disconnect_flag;
	bool failed_connect_flag;
	bool recv_flag;
	bool send_flag;

	//send()/recv() buffers
	buffer recv_buff;
	buffer send_buff;

	//how many bytes were added to recv_buff last recv() call
	int latest_recv;

	//how many bytes were added to send_buff last send() call
	int latest_send;

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
	seen:
		Updates last_active. Used for timeouts. Used by reactor but harmless to
		call from outside the reactor (although this has no meaning).
	timed_out:
		Returns true if the socket has timed out.
	*/
	void seen();
	bool timed_out();

private:
	//last time seen (used for timeouts)
	std::time_t last_active;
};
}//end of network namespace
#endif
