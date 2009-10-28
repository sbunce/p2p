#ifndef H_SLOT_MANAGER
#define H_SLOT_MANAGER

//custom
#include "share.hpp"
#include "slot.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>

//standard
#include <vector>

class slot_manager : private boost::noncopyable
{
public:
	slot_manager();

	/*
	recv:
		When the front of the recv_buff contains a slot related message it is
		passed to this function which will process the message and erase it from
		the buffer. Returns false if the host violated the protocol.
		Precondition: !recv_buff.empty() && recv_buff[0] = slot related message
	send:
		Appends a slot related message to send_buff. Returns true if send_buff had
		request appended.
	*/
	bool recv(network::buffer & recv_buff);
	bool send(network::buffer & send_buff);

private:
	/*
	The vector index of the element is the slot ID. This vector will always be
	sized such that it's no bigger than the highest slot. Slots will always be
	assigned lowest first to save memory. If a slot is closed and there is a
	higher slot still open the slot closed is set to an empty shared_ptr.
	*/
	std::vector<boost::shared_ptr<slot> > Outgoing_Slot;
	std::vector<boost::shared_ptr<slot> > Incoming_Slot;

	/*
	Pending_Slot_Request:
		Contains slots for which a slot request hasn't yet been made. Slot
		elements may get backed up if there are 256 slots open.
	Slot_Request:
		Slot elements for which we've requested a slot are pushed on the back of
		this. When a response arrives it will be for the element on the front of
		this.
	*/
	std::deque<boost::shared_ptr<slot> > Pending_Slot_Request;
	std::deque<boost::shared_ptr<slot> > Slot_Request;

	/*
	slot_counter:
		Slots are serviced in a round-robin fashion by servicing
	pending_block_requests:
		How many block requests are in the Send_Queue. This will not exceed
		protocol::PIPELINE_SIZE.
	*/
	unsigned slot_counter;
	unsigned pending_block_requests;

	/*
	When a message needs to be sent it is inserted in to the Send_Queue. Some
	messages are prioritized by being inserted closer to the front of the queue.

	Priority:
		The commands are also priorities. The higher the command number the higher
	the priority.

	Explanation of Priority:
		Establishing a slot to download is prioritized above all else because we
	want to get the download running ASAP. Sending the SLOT_ID to a remote host
	is prioritized next because we want uploads to get started ASAP.
		The requests for blocks are prioritized above sending hash_tree and file
	blocks to avoid a starvation problem that can happen when two hosts with
	asymmetric connections download from eachother concurrently. This starvation
	problem happens without prioritization because requests for new data from the
	remote end get stuck behind sending the huge hash_tree and file blocks.

	Insertion Algorithm:
	1. Insert message at the end of the deque.
	2. If message at current location - 1 is lower priority then swap. If current
	   location is beginning, or location - 1 is equal or greater priority then
	   stop.
	3. goto 2
	*/
	class send_queue_element
	{
	public:
		/*
		The slot_element which expects the response.
		Note: This may be empty if the slot_element was removed before a response
			was received.
		*/
		boost::shared_ptr<slot> SE;

		/*
		The bytes to send.
		Note: All but the first byte of this (the command) is cleared when
			send_queue_element put in Sent_Queue to save memory.
		*/
		network::buffer send_buff;

		/*
		Expected response associated with the size of the response.
		Note: This will be empty if no response is expected.
		*/
		std::vector<std::pair<unsigned char, int> > Expected_Response;
	};
	std::deque<boost::shared_ptr<send_queue_element> > Send_Queue;

	/*
	When a Send_Queue element gets used by the send() function the slot_element
	is pushed on the back of the Sent_Queue IFF a response is expected. The
	recv() function looks at the front of the Sent_Queue when receiving a
	response to know what slot the response is for.
	*/
	std::deque<boost::shared_ptr<send_queue_element> > Sent_Queue;
};
#endif
