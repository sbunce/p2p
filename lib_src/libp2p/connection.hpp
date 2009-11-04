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

	/*
	key_exchange_recv_call_back:
		The recv call back used during key exchange.
	recv_call_back:
		The recv call back used after key exchange complete.
	send_call_back:
		The send call back used after key exchange complete.
	*/
	void key_exchange_recv_call_back(network::connection_info & CI);
	void recv_call_back(network::connection_info & CI);
	void send_call_back(network::connection_info & CI);

private:
	network::proactor & Proactor;
	encryption Encryption;
	slot_manager Slot_Manager;
	int blacklist_state;
};
#endif
