#ifndef H_EXCHANGE_UDP
#define H_EXCHANGE_UDP

//custom
#include "db_all.hpp"
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
		const net::endpoint & endpoint,
		boost::function<void()> timeout_call_back = boost::function<void()>());
	void send(boost::shared_ptr<message_udp::send::base> M,
		const net::endpoint & endpoint);

	/* Info
	download_rate:
		Returns average download rate (bytes/second).
	upload_rate:
		Returns average upload rate (bytes/second).
	*/
	unsigned download_rate();
	unsigned upload_rate();

private:
	net::ndgram ndgram;
	net::select select;
	net::speed_calc Download, Upload;

	class expect_response_element
	{
	public:
		expect_response_element(
			boost::shared_ptr<message_udp::recv::base> message_in,
			boost::function<void()> timeout_call_back_in
		);
		boost::shared_ptr<message_udp::recv::base> message;
		boost::function<void()> timeout_call_back;
		//returns true if timed out
		bool timed_out();
	private:
		std::time_t time_first_expected;
	};

	/*
	Expect_Response:
		Incoming messages that are expected responses we expect to requests we've
		made. After a response is received it is removed from this container.
	Expect_Anytime:
		Incoming messages that we expect anytime. These are not responses.
	Send_Queue:
		If OS buffer becomes full messages to send are queued.
	*/
	std::multimap<net::endpoint, expect_response_element> Expect_Response;
	std::list<boost::shared_ptr<message_udp::recv::base> > Expect_Anytime;
	std::list<std::pair<boost::shared_ptr<message_udp::send::base>, net::endpoint> > Send_Queue;

	/*
	check_timeouts:
		Checks for timeouts and does timeout call backs.
	*/
	void check_timeouts();
};
#endif
