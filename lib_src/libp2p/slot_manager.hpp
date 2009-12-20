#ifndef H_SLOT_MANAGER
#define H_SLOT_MANAGER

//custom
#include "message.hpp"
#include "share.hpp"
#include "slot.hpp"

//include
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <convert.hpp>
#include <network/network.hpp>

class slot_manager : private boost::noncopyable
{
public:
	slot_manager(
		boost::function<void (boost::shared_ptr<message::base>)> expect_in,
		boost::function<void (boost::shared_ptr<message::base>)> send_in
	);

	/*
	recv_request_slot:
		Handles incoming request_slot messages.
	recv_request_slot_failed:
		Handles an ERROR in response to a request_slot.
	recv_slot:
		Handles incoming slot messages. The hash parameter is the hash of the file
		requested.
	resume:
		When the peer_ID is received this function is called to resume downloads.
	*/
	bool recv_request_slot(boost::shared_ptr<message::base> M);
	bool recv_request_slot_failed(boost::shared_ptr<message::base> M);
	bool recv_slot(boost::shared_ptr<message::base> M, const std::string hash);
	void resume(const std::string & peer_ID);

private:
	boost::function<void (boost::shared_ptr<message::base>)> expect;
	boost::function<void (boost::shared_ptr<message::base>)> send;

	/*
	Outgoing_Slot holds slots we have opened with the remote host.
	Incoming_Slot container holds slots the remote host has opened with us.
	Note: Index in vector is slot ID.
	Note: If there is an empty shared_ptr it means the slot ID is available.
	*/
	std::map<unsigned char, boost::shared_ptr<slot> > Outgoing_Slot;
	std::map<unsigned char, boost::shared_ptr<slot> > Incoming_Slot;

	/*
	open_slots:
		Number of slots opened, or requested, from the remote host.
		Invariant: 0 <= open_slots <= 256.
	Pending_Slot_Request:
		Slots which need to be opened with the remote host.
	*/
	unsigned open_slots;

//DEBUG, this should contain hashes, not slots
	std::list<boost::shared_ptr<slot> > Pending_Slot_Request;

	/*
	make_slot_requests:
		Does pending slot requests.
	*/
	void make_slot_requests();
};
#endif
