#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "share.hpp"

//include
#include <boost/utility.hpp>
#include <network/network.hpp>

class connection : private boost::noncopyable
{
public:
	connection(network::sock & S);

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
	encryption Encryption;

	//key exchange state
	enum exchange {
		INITIAL,
		SENT_PRIME_AND_LOCAL_RESULT,
		COMPLETE
	} Exchange;
};
#endif
