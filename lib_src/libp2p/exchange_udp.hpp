#ifndef H_EXCHANGE_UDP
#define H_EXCHANGE_UDP

//custom
#include "database.hpp"
#include "message_udp.hpp"

//include
#include <boost/utility.hpp>

class exchange_udp : private boost::noncopyable
{
public:
	exchange_udp();

	/*
	recv:
		Called to recv messages. Blocks for a short time if no messages to recv.
	*/
	void recv();

	/*
	expect_anytime:
		Expect incoming message anytime.
	expect_anytime_remove:
		Removes messages expected anytime.
	expect_response:
		After sending a message that expects a response this function should be
		called with the message expected.
	send:
		Sends a message.
	*/
	void expect_anytime(boost::shared_ptr<message_udp::recv::base> M);
	void expect_anytime_remove(boost::shared_ptr<message_udp::send::base> M);
	void expect_response(boost::shared_ptr<message_udp::recv::base> M);
	void send(boost::shared_ptr<message_udp::send::base> M,
		const network::endpoint & endpoint);

private:
	network::ndgram ndgram;
	network::select select;

	/*
	Incoming messages that are expected responses we expect to requests we've
	made. After a response is received it is removed from this container.
	*/
	std::list<boost::shared_ptr<message_udp::recv::base> > Expect_Response;

	/*
	Incoming messages that we expect anytime. These messages are not responses.
	*/
	std::list<boost::shared_ptr<message_udp::recv::base> > Expect_Anytime;
};
#endif
