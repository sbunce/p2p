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

	const std::string IP;
	const int connection_ID;

private:
	//locks all entry points to the object
	boost::recursive_mutex Recursive_Mutex;

	/*
	When a message is sent that expects a response it is pushed on the back of
	Expected. The request member of the message_pair stores the request made
	(but only if it's later needed). The response member is used to parse the
	incoming response.
	*/
	std::list<message::pair> Expected;

	/*
	Incoming messages that aren't responses are processed by the messages in this
	container. The message objects in this container are saved.
	*/
	std::vector<boost::shared_ptr<message::base> > Non_Response;

	network::proactor & Proactor;
	encryption Encryption;     //key exchange and stream cypher
	slot_manager Slot_Manager; //logic for dealing with slots
	std::string peer_ID;       //holds peer_ID when it's received
	int blacklist_state;       //used to know if blacklist updated

	/*
	recv_call_back:
		Proactor calls back to this function whenever data is received.
	send:
		Send a message and/or set up an expected response.
		Note: M_send and/or M_response may be empty.
	send_initial:
		Send initial messages. Called after key exchange completed.
	*/
	void recv_call_back(network::connection_info & CI);
	void send(boost::shared_ptr<message::base> M_send,
		boost::shared_ptr<message::base> M_response);
	void send_initial();
};
#endif
