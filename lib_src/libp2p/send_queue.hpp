#ifndef H_SEND_QUEUE
#define H_SEND_QUEUE

//custom
#include "slot.hpp"

//standard
#include <list>

class send_queue
{
public:
	/*
	expected:
		Returns true and sets size if message was expected.
//DEBUG, have checks to determine if message is a response, if it is then
//check Sent. Else just verify it's the correct size.
	insert:
		Add message to be sent.
	send:
		Returns message to send, or empty shared_ptr if there is nothing to send.
		Postcondition: message is put in Sent list if it expects response.
	*/
	bool expected(const network::buffer & recv_buf, unsigned & size);
	void insert(boost::shared_ptr<slot::message> & M);
	boost::shared_ptr<slot::message> send();

private:
	//pending messages to send
	std::list<boost::shared_ptr<slot::message> > Send;

	/*
	When a message is sent that expects a response it is pushed on to the back of
	this. When a response to a slot related message is recieved it is always to
	the element on the front of this queue.
	*/
	std::list<boost::shared_ptr<slot::message> > Sent;
};
#endif
