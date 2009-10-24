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
		//network::sock & S,
		share & Share_in
	);

	const std::string IP;
	const std::string port;

	/*
	key_exchange_recv_call_back:
		The recv call back used during key exchange.
	key_exchange_send_call_back:
		The send call back used during key exchange.
	recv_call_back:
		The recv call back used after key exchange complete.
	send_call_back:
		The send call back used after key exchange complete.
	*/
	void key_exchange_recv_call_back(/*network::sock & S*/);
	void key_exchange_send_call_back(/*network::sock & S*/);
	void recv_call_back(/*network::sock & S*/);
	void send_call_back(/*network::sock & S*/);

private:
	encryption Encryption;

	//see documentation in database::table::blacklist
	int blacklist_state;

	//manages all slot related messages
	slot_manager Slot_Manager;
};
#endif
