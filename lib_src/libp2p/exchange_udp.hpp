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
	tick:
		Called to recv messages and do other tasks. Blocks for a short time if no
		messages to recv.
	*/
	void tick();

	/*
	expect_anytime:
		Expect incoming message anytime.
	expect_anytime_remove:
		Removes messages expected anytime.
	expect_response:
		After sending a message that expects a response this function should be
		called with the message expected. Optionally, a timeout call back can be
		specified.
	send:
		Sends a message.
	*/
	void expect_anytime(boost::shared_ptr<message_udp::recv::base> M);
	void expect_anytime_remove(boost::shared_ptr<message_udp::send::base> M);
	void expect_response(boost::shared_ptr<message_udp::recv::base> M,
		const network::endpoint & endpoint,
		boost::function<void()> timeout_call_back = boost::function<void()>());
	void send(boost::shared_ptr<message_udp::send::base> M,
		const network::endpoint & endpoint);

private:
	network::ndgram ndgram;
	network::select select;

	class expect_response_element
	{
	public:
		expect_response_element(
			boost::shared_ptr<message_udp::recv::base> message_in,
			boost::function<void()> timeout_call_back_in
		);

		boost::shared_ptr<message_udp::recv::base> message;
		boost::function<void()> timeout_call_back;

		/*
		timed_out:
			Returns true if timed out.
		*/
		bool timed_out();

	private:
		std::time_t time_first_expected;
	};

	/*
	Incoming messages that are expected responses we expect to requests we've
	made. After a response is received it is removed from this container.
	*/
	std::multimap<network::endpoint, expect_response_element> Expect_Response;

	/*
	Incoming messages that we expect anytime. These messages are not responses.
	*/
	std::list<boost::shared_ptr<message_udp::recv::base> > Expect_Anytime;

	/*
	check_timeouts:
		Checks for timeouts and does timeout call backs.
	*/
	void check_timeouts();
};
#endif
