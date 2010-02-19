#ifndef H_SLOT_MANAGER
#define H_SLOT_MANAGER

//custom
#include "exchange_tcp.hpp"
#include "message_tcp.hpp"
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
		exchange_tcp & Exchange_in,
		boost::function<void(const int)> trigger_tick_in
	);
	~slot_manager();

	/*
	empty:
		Returns true if no incoming/outgoing slots.
	remove:
		Remove incoming/outgoing slots with the specified hash. Cancel pending
		slots with the specified hash.
	resume:
		When the peer_ID is received this function is called to resume downloads.
	tick:
		Called after exchange done processing buffers. Does periodic tasks.
	*/
	bool empty();
	void remove(const std::string & hash);
	void resume(const std::string & peer_ID);
	void tick();

private:
	exchange_tcp & Exchange;
	boost::function<void(const int)> trigger_tick;

	/*
	Incoming_Slot container holds slots the remote host has opened with us.
	Outgoing_Slot holds slots we have opened with the remote host.
	Note: Index in vector is slot ID.
	Note: If there is an empty shared_ptr it means the slot ID is available.
	*/
	std::map<unsigned char, boost::shared_ptr<slot> > Incoming_Slot;
	std::map<unsigned char, boost::shared_ptr<slot> > Outgoing_Slot;

	//current pipeline size (unfulfilled requests)
	unsigned pipeline_size;

	/*
	When making requests for blocks we loop through slots. This keeps track of
	the latest slot that had a block sent.
	*/
	unsigned char latest_slot;

	/*
	open_slots:
		Number of outgoing slots opened, or requested, from the remote host.
		Invariant: 0 <= open_slots <= 256.
	Pending:
		Hashes of files that need to be downloaded from host. Files will be
		queued up if the slot limit of 256 is reached.
	*/
	unsigned open_slots;
	std::list<std::string> Pending;

	/*
	close_complete:
		Send close_slot messages for complete slots.
	send_block_requests:
		Makes any hash_tree of file block requests that need to be done.
	send_have:
		Sends have_* messages.
	send_slot_requests:
		Does pending slot requests.
	*/
	void close_complete();
	void send_block_requests();
	void send_have();
	void send_slot_requests();

	/* Receive Functions
	recv_close_slot:
		Call back for receiving close slot messages.
	recv_file_block:
		Call back for receiving a file block.
	recv_have_file_block:
		Called when a have_file_block_message is received.
	recv_have_hash_tree_block:
		Called when a have_hash_tree_block message is received.
	recv_hash_tree_block:
		Call back for receiving a hash tree block.
	recv_request_hash_tree_block:
		Call back for receiving request for hash tree block.
	recv_request_block_failed:
		Call back for when a hash tree block request or file block request fails.
	recv_request_slot:
		Handles incoming request_slot messages.
	recv_request_slot_failed:
		Handles an ERROR in response to a request_slot.
	recv_slot:
		Handles incoming slot messages. The hash parameter is the hash of the file
		requested.
	*/
	bool recv_close_slot(network::buffer & buf);
	bool recv_file_block(network::buffer & buf,
		const unsigned slot_num, const boost::uint64_t block_num);
	bool recv_have_file_block(network::buffer & buf);
	bool recv_have_hash_tree_block(network::buffer & buf);
	bool recv_hash_tree_block(network::buffer & buf,
		const unsigned slot_num, const boost::uint64_t block_num);
	bool recv_request_hash_tree_block(network::buffer & buf,
		const unsigned slot_num);
	bool recv_request_file_block(network::buffer & buf,
		const unsigned slot_num);
	bool recv_request_block_failed(network::buffer & buf,
		const unsigned slot_num);
	bool recv_request_slot(network::buffer & buf);
	bool recv_request_slot_failed(network::buffer & buf);
	bool recv_slot(network::buffer & buf, const std::string hash);
};
#endif
