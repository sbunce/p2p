#ifndef H_NET_CONNECTION
#define H_NET_CONNECTION

//custom
#include "ID_manager.hpp"

//include
#include <net/endpoint.hpp>
#include <net/nstream.hpp>
#include <net/proactor.hpp>

//standard
#include <set>

namespace net{
class connection : private boost::noncopyable
{
private:
	//amount of time a connection is allowed to be half-open
	static const unsigned connect_timeout = 30;

	/*
	Timeout (seconds) before idle sockets disconnected. This has to be quite high
	otherwise client that download very slowly will be erroneously disconnected.
	*/
	static const unsigned idle_timeout = 600;

	bool connected;          //false if async connect in progress
	ID_manager & ID_Manager; //allocates and deallocates connection_ID
	std::time_t time;        //last time there was activity on socket
	int socket_FD;           //file descriptor, will change after open_async()

public:
	//ctor for outgoing connection (starts async connect)
	connection(
		ID_manager & ID_Manager_in,
		const endpoint & ep
	);
	//ctor for incoming connection
	connection(
		ID_manager & ID_Manager_in,
		const boost::shared_ptr<nstream> & N_in
	);
	~connection();

	//modified by proactor
	bool disc_on_empty; //if true we disconnect when send_buf is empty
	buffer send_buf;    //bytes to send appended to this

	//const
	const boost::shared_ptr<nstream> N; //connection object
	const int connection_ID;            //see above

	/* WARNING
	This should not be modified by any thread other than a dispatcher
	thread. Modifying this from the network_thread is not thread safe unless
	it hasn't yet been passed to dispatcher.
	*/
	boost::shared_ptr<proactor::connection_info> CI;

	/*
	half_open:
		Returns true if async connect in progress.
	is_open:
		Returns true if async connecte succeeded. This is called after half
		open socket becomes writeable.
		Postcondition: If true returned then half_open() will return false.
	socket:
		Returns file descriptor.
	timed_out:
		Returns true if connection timed out.
	touch:
		Resets timer used for timeout.
	*/
	bool half_open();
	bool is_open();
	int socket();
	bool timeout();
	void touch();
};
}//end namespace net
#endif
