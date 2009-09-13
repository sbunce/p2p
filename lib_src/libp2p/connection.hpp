#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "shared_files.hpp"
#include "slot_manager.hpp"

//include
#include <boost/utility.hpp>
#include <network/network.hpp>

class connection : private boost::noncopyable
{
public:
	connection(network::sock & S);

	const std::string IP;
	const std::string port;

	/*
	process_pending_send:
		Handles appending to the send buffer.
	recv_call_back:
		After any bytes are received a call back will be done to this function.
	send_call_back:
		After any bytes are sent a call back will be done to this function.
	*/
	void process_pending_send(network::sock & S);
	void recv_call_back(network::sock & S);
	void send_call_back(network::sock & S);

private:
	//key exchange state
	enum exchange {
		initial,
		sent_prime_and_local_result,
		complete
	} Exchange;

	encryption Encryption;

	//see documentation in database::table::blacklist
	int blacklist_state;

	//manages all slot related messages
	slot_manager Slot_Manager;
};
#endif
