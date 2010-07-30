#ifndef H_K_CONTACT
#define H_K_CONTACT

//custom
#include "protocol_udp.hpp"

/*
This class models interaction between requester/responder via UDP. The send()
function says when we should send a message. The timeout() function says when
we've waited too long for a response and have decided the request was lost.

The touch() function can be used in situations where we want to poll a remote
host to verify that it's available (pings).

The timeout_count() function can be used to implement retransmission limits.
*/
class k_contact : private boost::noncopyable
{
public:
	/*
	touch_timeout:
		Timeout set when touch() called (seconds).
	delay:
		The send() function won't return true for this many seconds.
	*/
	k_contact(
		const unsigned touch_timeout_in = 0,
		const unsigned delay = 0
	);
	k_contact(const k_contact & C);

	/*
	send:
		Returns true if message can be sent. This function uses delay.
		Postcondition: Timeout set for protocol_udp::response_timeout seconds.
	send_immediate:
		Ignore delay, send immediately.
		Postcondition: Timeout set for protocol_udp::response_timeout seconds.
	timeout:
		Returns true if contact has timed out.
		Postcondition: touch() called.
		Postcondition: delay set to 0.
		Postcondition: timeout_count() returns one greater.
	timeout_count:
		Returns number of times contact has timed out.
	touch:
		Update time used for timeout.
		Postcondition: Timeout set for protocol_udp::response_timeout seconds.
	*/
	bool send();
	bool send_immediate();
	bool timeout();
	unsigned timeout_count();
	void touch();

private:
	unsigned touch_timeout;
	std::time_t time;
	bool sent;
	unsigned timeout_cnt;
};
#endif
