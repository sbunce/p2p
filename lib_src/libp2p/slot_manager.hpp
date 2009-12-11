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
	slot_manager(std::list<network::buffer> & Send_in);

	/*
	recv_request_slot:
		Handles incoming REQUEST_SLOT messages.
	recv_request_slot_failed:
		Handles an ERROR in response to a REQUEST_SLOT.
	recv_slot:
		Handles incoming SLOT messages.
	*/
	bool recv_request_slot(network::connection_info & CI);
	bool recv_request_slot_failed(network::connection_info & CI);
	bool recv_slot(network::connection_info & CI);

	/*
	resume:
		When the peer_ID is received this function is called to resume downloads.
	*/
	void resume(network::connection_info & CI, const std::string & peer_ID);

private:
	//reference to connection::Send
	std::list<network::buffer> & Send;

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
	std::list<boost::shared_ptr<slot> > Pending_Slot_Request;
	std::list<boost::shared_ptr<slot> > Slot_Request;

	/*
	make_slot_requests:
		Does pending slot requests.
	*/
	void make_slot_requests(network::connection_info & CI);
};
#endif
