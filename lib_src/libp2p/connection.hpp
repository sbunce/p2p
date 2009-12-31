#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "message.hpp"
#include "share.hpp"
#include "slot_manager.hpp"

//include
#include <boost/utility.hpp>
#include <network/network.hpp>

class connection : private boost::noncopyable
{
public:
	connection(
		network::proactor & Proactor_in,
		network::connection_info & CI
	);

	const int connection_ID;
	const std::string IP;

private:
	//locks all entry points to the object
	boost::recursive_mutex Recursive_Mutex;

	network::proactor & Proactor;

	/*
	Expected responses are pushed on the back of Expect after a request is sent.
	When a response arrives it is for the message on the front of Expect.
	*/
	std::list<boost::shared_ptr<message::base> > Expect_Response;

	/*
	Incoming messages that aren't responses are processed by the messages in this
	container. The message objects in this container are reused.
	*/
	std::list<boost::shared_ptr<message::base> > Expect_Anytime;

	encryption Encryption; //key exchange and stream cypher
	std::string peer_ID;   //holds peer_ID when it's received
	int blacklist_state;   //used to know if blacklist updated

	//does everything slot related
	slot_manager Slot_Manager;

	/*
	expect_response:
		After sending a message that expects a response this function should be
		called with the message expected. If there are multiple possible messages
		expected a composite message should be used.
	expect_anytime:
		Expect a incoming message at any time.
	expect_anytime_remove:
		Removes messages expected anytime that expects the specified buffer.
	send:
		Sends a message. Handles encryption.
	*/
	void expect_response(boost::shared_ptr<message::base> M);
	void expect_anytime(boost::shared_ptr<message::base> M);
	void expect_anytime_erase(network::buffer buf);
	void send(boost::shared_ptr<message::base> M);

	/*
	proactor_recv_call_back:
		Proactor calls back to this function whenever data is received.
	send:
		Send a message and/or set up an expected response.
	send_initial:
		Send initial message. Called after key exchange completed.
	*/
	void proactor_recv_call_back(network::connection_info & CI);
	void send_initial();

	/* Functions to handle receiving messages.
	Note: These functions return false if the protocol was violated.
	*/
	bool recv_initial(boost::shared_ptr<message::base> M);
	bool recv_p_rA(boost::shared_ptr<message::base> M, network::connection_info & CI);
	bool recv_rB(boost::shared_ptr<message::base> M, network::connection_info & CI);
};
#endif
