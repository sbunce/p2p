#ifndef H_SLOT
#define H_SLOT

//custom
#include "slot_element.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>

//standard
#include <vector>

class slot : private boost::noncopyable
{
public:
	slot();

	/*
	add_incoming_slot:
		Remote host wants to download from us. Allocate a slot for them and append
		the response to send_buff. Returns false if protocol was violated.
	add_outgoing_slot:
		We want to download something from a remote host. Schedules a slot
		request.
	message:
		Slot related messages are passed to this function. This function returns
		false if the host violated the protocol.
	request:
		Appends a slot related request to send_buff. This can be a slot request,
		a request for a hash block, or a request for a file block. Returns true if
		send_buff had request appended.
	*/
	bool add_incoming_slot(boost::shared_ptr<slot_element> SE, network::buffer & send_buff);
	void add_outgoing_slot(boost::shared_ptr<slot_element> SE);
	bool message(network::buffer & recv_buff);
	bool request(network::buffer & send_buff);

private:
	/*
	The vector index of the element is the slot ID. This vector will always be
	sized such that it's no bigger than the highest slot. Slots will always be
	assigned lowest first to save memory. If a slot is closed and there is a
	higher slot still open the slot closed is set to an empty shared_ptr.
	*/
	std::vector<boost::shared_ptr<slot_element> > Outgoing_Slot;
	std::vector<boost::shared_ptr<slot_element> > Incoming_Slot;

	/*
	Pending_Slot_Request:
		The add_outgoing_slot function pushes slot elements on the back of this.
		The request function will take slot elements from the front of this and
		make slot requests. Slot elements may get backed up if there are 256 slots
		open.
	Slot_Request:
		Slot elements for which we've requested a slot are pushed on the back of
		this. When a response arrives it will be for the element on the front of
		this.
	*/
	std::deque<boost::shared_ptr<slot_element> > Pending_Slot_Request;
	std::deque<boost::shared_ptr<slot_element> > Slot_Request;
};
#endif
