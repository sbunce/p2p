#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "share.hpp"
#include "slot_manager.hpp"

//include
#include <boost/utility.hpp>
#include <network/network.hpp>

class connection : private boost::noncopyable
{
public:
	connection(
		network::proactor & Proactor_in,
		network::connection_info & CI
	);

	const std::string IP;

private:
	//locks all entry points in to the object (including call backs)
	boost::recursive_mutex Recursive_Mutex;

	/*
	Messages to send are inserted in to the Send buffer. When a message that
	expects a response is sent it is pushed on to the back of the Sent buffer.
	Incoming messages that are responses are matched to the request on the front
	of the Sent buffer.
	*/
	std::list<network::buffer> Send;
	std::list<network::buffer> Sent;

	network::proactor & Proactor;
	encryption Encryption;
	slot_manager Slot_Manager;
	std::string peer_ID; //holds peer_ID when it's received
	int blacklist_state; //used to know if blacklist updated

	/* Proactor Call Backs
	key_exchange_recv_call_back:
		Used for key exchange. This is the first call back used.
	initial_recv_call_back:
		Receives initial messages sent after key exchange.
	recv_call_back:
		The last recv call back used. Handles commands.
	*/
	void key_exchange_recv_call_back(network::connection_info & CI);
	void initial_recv_call_back(network::connection_info & CI);
	void recv_call_back(network::connection_info & CI);

	/*
	send_initial:
		Sends initial messages after key exchange done.
	*/
	void initial_send(network::connection_info & CI);
};
#endif
