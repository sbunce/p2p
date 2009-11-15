#ifndef H_SLOT_MANAGER
#define H_SLOT_MANAGER

//custom
#include "share.hpp"
#include "slot.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <convert.hpp>
#include <network/network.hpp>

//standard
#include <vector>


class slot_manager : private boost::noncopyable
{
public:
	slot_manager(network::connection_info & CI);

	/*
	is_slot_command:
		Returns true if specified command is a slot related command.
	recv:
		Removes/processes one message on front of CI.recv_buf. Returns false if
		there is an incomplete message on front of recv_buf.
		Precondition: !CI.recv_buf.empty() && is_slot_command(CI.recv_buf[0]) = true
	*/
	static bool is_slot_command(const unsigned char command);
	bool recv(network::connection_info & CI);

private:
	//used to detect when slot within share modified
	int share_slot_state;

	/*
	Outgoing_Slot holds slots we have opened with the remote host.
	Incoming_Slot container holds slots the remote host has opened with us.
	Note: Index in vector is slot ID.
	Note: If there is an empty shared_ptr it means the slot ID is available.
	*/
	std::map<unsigned char, boost::shared_ptr<slot> > Outgoing_Slot;
	std::map<unsigned char, boost::shared_ptr<slot> > Incoming_Slot;

	/*
	Pending_Slot_Request:
		Slots which need to be opened with the remote host.
	Slot_Request:
		Slot elements for which we've requested a slot are pushed on the back of
		this. When a response arrives it will be for the element on the front of
		this.
	*/
	std::deque<boost::shared_ptr<slot> > Pending_Slot_Request;
	std::deque<boost::shared_ptr<slot> > Slot_Request;

	//set of all slots that exist somewhere in this class
	std::set<boost::shared_ptr<slot> > Known_Slot;

	//messages that expect a response
	class message
	{
	public:
		boost::shared_ptr<slot> Slot; //slot that expects response (may be empty)
		network::buffer send_buf;     //message to send

		//possible responses expected (command associated with expected size)
		std::vector<std::pair<unsigned char, unsigned> > expected_response;
	};
	std::deque<boost::shared_ptr<message> > Send_Queue;

	/*
	When a message is sent that expects a response it is pushed on to the back of
	this. When a response to a slot related message is recieved it is always to
	the element on the front of this queue.
	*/
	std::deque<boost::shared_ptr<message> > Sent_Queue;

	/*
	sync_slots:
		Sync slots in the share with slots in the slot_manager. This is useful for
		when a slot becomes associated with a new host, or when a totally new slot
		is added.
	*/
	bool recv_request_slot(network::connection_info & CI);
	void send_close_slot(const unsigned char slot_ID);
	void sync_slots(network::connection_info & CI);
};
#endif
