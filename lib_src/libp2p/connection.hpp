#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "exchange.hpp"
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

	network::proactor & Proactor;
	encryption Encryption;
	exchange Exchange;
	slot_manager Slot_Manager;

	//peer_ID of remote host
	std::string peer_ID;

	//allows us to determine if the blacklist has changed
	int blacklist_state;

	/* Proactor Call Backs
	key_exchange_recv_call_back:
		Used for key exchange. This is the first call back used.
	initial_recv_call_back:
		Receives initial messages sent after key exchange.
	recv_call_back:
		The last recv call back used. Handles commands.
	send_call_back:
		The last send call back used. Handles sending new requests when send_buf
		empties out.
	*/
	void key_exchange_recv_call_back(network::connection_info & CI);
	void initial_recv_call_back(network::connection_info & CI);
	void recv_call_back(network::connection_info & CI);
	void send_call_back(network::connection_info & CI);

	/*
	send_initial:
		Sends initial messages after key exchange done.
	*/
	void initial_send(network::connection_info & CI);
};
#endif
