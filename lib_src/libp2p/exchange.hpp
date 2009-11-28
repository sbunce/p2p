#ifndef H_EXCHANGE
#define H_EXCHANGE

//custom
#include "database.hpp"
#include "protocol.hpp"

//include
#include <boost/optional.hpp>
#include <network/network.hpp>

//standard
#include <list>
#include <vector>

class exchange
{
public:
	//message to send over the network
	class message
	{
	public:
		/*
		Data put in send_buf when sending a message. When we recieve a response to
		a request the request will be in send_buf, and the response in recv_buf.
		*/
		network::buffer recv_buf;
		network::buffer send_buf;

		/*
		If the message to send expects a response the possible responses are
		stored here.
		std::pair<command, size of response>
		*/
//DEBUG, use this on SUBSCRIBE_* requests
//have exchange memorize the values
		std::vector<std::pair<unsigned char, unsigned> > expected_response;
	};

	/*
	recv:
		Returns shared_ptr to received message. Returns empty shared_ptr if a
		complete message hasn't been received or if the remote host was
		blacklisted.
	schedule_send:
		Add message to be sent.
	send:
		Returns message to send (or empty buffer if nothing to send).
	*/
	boost::shared_ptr<message> recv(network::connection_info & CI);
	void schedule_send(boost::shared_ptr<message> & M);
	network::buffer send();

private:
	//pending messages to send
	std::list<boost::shared_ptr<message> > Send;

	/*
	When a message is sent that expects a response it is pushed on to the back of
	this. When a response arrives it is to the message on the front.
	*/
	std::list<boost::shared_ptr<message> > Sent;
};
#endif
