#ifndef H_MESSAGE
#define H_MESSAGE

//custom
#include "encryption.hpp"
#include "protocol.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <network/network.hpp>

//standard
#include <list>
#include <map>

class message
{
public:
	enum message_type{
		key_exchange_p_rA,
		key_exchange_rB,
		initial,
		error,
		request_slot,
		slot,
		request_hash_tree_block,
		request_file_block,
		block,
		have_hash_tree_block,
		have_file_block,
		close_slot,
		none
	};

	network::buffer buf;

	/* Functions to parse incoming messages.
	expects:
		Returns true if parse() expects message on front of CI.recv_buf.
	parse:
		Returns true if incoming message received.
		Postcondition: response placed in buf.
	type:
		Returns the type of the message. (think RTTI)
	*/
	virtual bool expects(network::connection_info & CI) = 0;
	virtual bool parse(network::connection_info & CI) = 0;
	virtual message_type type() = 0;
};

/*
Used in place of a std::pair to be more expressive.
Ex: MP.response instead of MP.second.
*/
class message_pair
{
public:
	message_pair(){}

	message_pair(
		boost::shared_ptr<message> request_in,
		boost::shared_ptr<message> response_in
	):
		request(request_in),
		response(response_in)
	{}

	boost::shared_ptr<message> request;
	boost::shared_ptr<message> response;
};
#endif
