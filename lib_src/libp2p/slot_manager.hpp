#ifndef H_SLOT_MANAGER
#define H_SLOT_MANAGER

//custom
#include "exchange.hpp"
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
	slot_manager(exchange & Exchange_in);
	~slot_manager();

	/*
	resume:
		When the peer_ID is received this function is called to resume downloads.
	*/
	void resume(const std::string & peer_ID);

private:
	//reference to connection::Exchange
	exchange & Exchange;

	/*
	Incoming_Slot container holds slots the remote host has opened with us.
	Outgoing_Slot holds slots we have opened with the remote host.
	Note: Index in vector is slot ID.
	Note: If there is an empty shared_ptr it means the slot ID is available.
	*/
	std::map<unsigned char, boost::shared_ptr<slot> > Incoming_Slot;
	std::map<unsigned char, boost::shared_ptr<slot> > Outgoing_Slot;

	/*
	These are passed to, and adjusted by, the transfer objects for adaptive
	pipeline sizing.
	Note: These are only for pipelining of block requests.
	*/
	unsigned pipeline_cur; //current size (unfulfilled requests)
	unsigned pipeline_max; //current maximum size

	/*
	When making requests for blocks we loop through slots. This keeps track of
	the latest slot that had a block sent.
	*/
	unsigned char latest_slot;

	/*
	open_slots:
		Number of slots opened, or requested, from the remote host.
		Invariant: 0 <= open_slots <= 256.
	Pending:
		Hashes of files that need to be downloaded from host. Files will be
		queued up if the slot limit of 256 is reached.
	*/
	unsigned open_slots;
	std::queue<std::string> Pending;

	/*
	make_block_requests:
		Makes any hash_tree of file block requests that need to be done.
	make_slot_requests:
		Does pending slot requests.
	recv_file_block:
		Call back for receiving a file block.
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
	void make_block_requests(boost::shared_ptr<slot> S);
	void make_slot_requests();
	bool recv_file_block(boost::shared_ptr<message::base> M,
		const unsigned slot_num, const boost::uint64_t block_num);
	bool recv_hash_tree_block(boost::shared_ptr<message::base> M,
		const unsigned slot_num, const boost::uint64_t block_num);
	bool recv_request_hash_tree_block(boost::shared_ptr<message::base> M,
		const unsigned slot_num);
	bool recv_request_file_block(boost::shared_ptr<message::base> M,
		const unsigned slot_num);
	bool recv_request_block_failed(boost::shared_ptr<message::base> M,
		const unsigned slot_num);
	bool recv_request_slot(boost::shared_ptr<message::base> M);
	bool recv_request_slot_failed(boost::shared_ptr<message::base> M);
	bool recv_slot(boost::shared_ptr<message::base> M, const std::string hash);
};
#endif
