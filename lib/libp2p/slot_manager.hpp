#ifndef H_SLOT_MANAGER
#define H_SLOT_MANAGER

//custom
#include "exchange_tcp.hpp"
#include "message_tcp.hpp"
#include "protocol_tcp.hpp"
#include "share.hpp"
#include "slot.hpp"

//include
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <convert.hpp>
#include <net/net.hpp>

class slot_manager : private boost::noncopyable
{
public:
	slot_manager(
		exchange_tcp & Exchange_in,
		const boost::function<void(const net::endpoint & ep, const std::string & hash)> & peer_call_back_in,
		const boost::function<void(const int)> & trigger_tick_in
	);
	~slot_manager();

	/*
	add:
		Add hash of a file to download.
	empty:
		Returns true if no incoming/outgoing slots.
	remove:
		Remove incoming/outgoing slots with the specified hash. Cancel pending
		slots with the specified hash.
	set_remote_listen:
		When initial message is received this function is called to set the
		endpoint we can contact remote host on.
	tick:
		Called after exchange done processing buffers. Does periodic tasks.
	*/
	void add(const std::string & hash);
	bool empty();
	void remove(const std::string & hash);
	void set_remote_listen(const net::endpoint & ep);
	void tick();

private:
	exchange_tcp & Exchange;
	const boost::function<void(const net::endpoint & ep, const std::string hash)> peer_call_back;
	const boost::function<void(const int)> trigger_tick;

	/*
	Upload_Slot container holds slots the remote host has opened with us.
	Download_Slot holds slots we have opened with the remote host.
	Note: Index in vector is slot ID.
	Note: If there is an empty shared_ptr it means the slot ID is available.
	*/
	std::map<unsigned char, boost::shared_ptr<slot> > Upload_Slot;
	std::map<unsigned char, boost::shared_ptr<slot> > Download_Slot;

	//unfulfilled block requests we have made
	unsigned outgoing_pipeline_size;

	//unfulfilled block requests remote host has made
	unsigned incoming_pipeline_size;

	/*
	When making requests for blocks we loop through slots. This keeps track of
	the latest slot that had a block sent.
	*/
	unsigned char latest_slot;

	/*
	open_slots:
		Number of outgoing slots opened, or requested, from the remote host.
		Invariant: 0 <= open_slots <= 256.
	Hash_Pending:
		Hashes of files that need to be downloaded from host. Files will be
		queued up if the slot limit of 256 is reached.
	Hash_Opened:
		Hashes we've opened slots for.
	*/
	unsigned open_slots;
	std::set<std::string> Hash_Pending;
	std::set<std::string> Hash_Opened;

	/*
	This is the endpoint the remote host can be contacted on. This is used for
	sending peer messages. It is set by set_remote_listen().
	*/
	boost::scoped_ptr<net::endpoint> remote_listen;

	/*
	close_complete:
		Send close_slot messages for complete slots.
	send_block_requests:
		Makes any hash_tree of file block requests that need to be done.
	send_have:
		Sends have_* messages.
	send_peer:
		Sends peer_* messages.
	send_slot_requests:
		Does pending slot requests.
	*/
	void close_complete();
	void send_block_requests();
	void send_have();
	void send_peer();
	void send_slot_requests();

	/* Receive Functions
	Called when message received. Function named after message type it handles.
	*/
	bool recv_close_slot(const unsigned char slot_num);
	bool recv_file_block(const net::buffer & block,
		const unsigned char slot_num, const boost::uint64_t block_num);
	bool recv_have_file_block(const unsigned char slot_num,
		const boost::uint64_t block_num);
	bool recv_have_hash_tree_block(const unsigned char slot_num,
		const boost::uint64_t block_num);
	bool recv_hash_tree_block(const net::buffer & block,
		const unsigned char slot_num, const boost::uint64_t block_num);
	bool recv_peer(const unsigned char slot_num, const net::endpoint & ep);
	bool recv_request_hash_tree_block(const unsigned char slot_num,
		const boost::uint64_t block_num);
	bool recv_request_file_block(const unsigned char slot_num,
		const boost::uint64_t block_num);
	bool recv_request_block_failed(const unsigned char slot_num);
	bool recv_request_slot(const std::string & hash);
	bool recv_request_slot_failed();
	bool recv_slot(const unsigned char slot_num, const boost::uint64_t file_size,
		const std::string & root_hash, bit_field & tree_BF, bit_field & file_BF,
		const std::string hash);

	/* Send Functions (called after message sent)
	sent_block:
		Called when we finish sending a block.
	*/
	void sent_block();
};
#endif
