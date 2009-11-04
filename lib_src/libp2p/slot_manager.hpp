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
		Processes slot related message on front of CI.recv_buf. Returns true and
		removes the message from the buffer if a complete message was received.
		Returns false if there was only a partial message in CI.recv_buf.
		Precondition: !CI.recv_buf.empty() && is_slot_command(CI.recv_buf[0]) = true
	send:
		Called by connection every time the send_call_back is called. This may or
		may not append data to send_buf.
	*/
	static bool is_slot_command(const unsigned char command);
	bool recv(network::connection_info & CI);
	void send(network::buffer & send_buf);

private:
	const int connection_ID;
	const std::string IP;
	const std::string port;

	//used to detect when slot within share modified
	int share_slot_state;

	/*
	The slot ID mapped to a slot. The Outgoing_Slot container holds slots we have
	opened with the remote host. The Incoming_Slot container holds slots the
	remote host has opened with us.

	If the Incoming_Slot container has an empty shared_ptr it means the slot was
	closed and we should return FILE_REMOVED for any requests until we get a
	CLOSE_SLOT at which point we can remove the Incoming_Slot element.
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

	/*
	Set of all slots in all containers in this class. Used when synchronizing
	slots with the share.
	*/
	std::set<boost::shared_ptr<slot> > Known_Slot;

	//when a message needs to be sent it is inserted in to the Send_Queue
	class message
	{
	public:
		//slot which expects response, empty if no response expected
		boost::shared_ptr<slot> Slot;

		/*
		Request to send.
		Note: When a block request is sent we need to know if it is for a hash
		tree block or file block so we know where to send the block when it
		arrives. To determine this we look at the first byte in send_buf.
		*/
		network::buffer send_buf;

		//possible responses expected (command associated with size of message)
		std::vector<std::pair<unsigned char, int> > expected_response;
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
	void sync_slots();

	/* Receive Functions

	*/

	/* Send Functions
	There functions are all named after the commands they send. Refer to the
	protocol documentation to know what these do.
	*/
	void send_close_slot(const unsigned char slot_ID);
	void send_request_slot(const std::string & hash);
	void send_request_slot_failed();
	void send_request_block(std::pair<unsigned char, boost::shared_ptr<slot> > & P);
	void send_file_removed();
	void send_slot_ID(std::pair<unsigned char, boost::shared_ptr<slot> > & P);
};
#endif
