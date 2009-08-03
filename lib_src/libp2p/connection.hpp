#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "prime_generator.hpp"
#include "share.hpp"

//include
#include <network/network.hpp>

class connection
{
public:
	connection(
		network::sock & S,
		prime_generator & Prime_Generator_in
	);

	/*
	recv_call_back:
		After any bytes are received a call back will be done to this function.
	send_call_back:
		After any bytes are sent a call back will be done to this function.
	*/
	void recv_call_back(network::sock & S);
	void send_call_back(network::sock & S);

private:
	prime_generator & Prime_Generator;
	encryption Encryption;
};
#endif
