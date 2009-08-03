#ifndef H_CONNECTION
#define H_CONNECTION

//include
#include <network/network.hpp>

class connection
{
public:
	connection();

	/*
	recv_call_back:
		After any bytes are received a call back will be done to this function.
	send_call_back:
		After any bytes are sent a call back will be done to this function.
	*/
	void recv_call_back(network::sock & S);
	void send_call_back(network::sock & S);

private:

};
#endif
