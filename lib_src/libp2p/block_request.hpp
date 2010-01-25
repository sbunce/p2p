/*
The block_request class keeps track of what blocks we have and what blocks
remote hosts have. It also can determine the rarest blocks to request.

This class is threadsafe.
*/
#ifndef H_BLOCK_REQUEST
#define H_BLOCK_REQUEST

//custom
#include "protocol.hpp"

//include
#include <bit_field.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <algorithm>
#include <limits>
#include <map>
#include <set>

class block_request : private boost::noncopyable
{
public:
	block_request(const boost::uint64_t block_count_in);

	/*
	add_block_local (one paramter):
		Add block. Used when next_request was not involved in getting a block.
		This function is used during hash check to mark what blocks we already
		have.
		Precondition: No hosts must be subscribed.
	add_block_local (two parameters):
		Add block from specific host.
	add_block_local_all:
		Adds all local blocks.
		Postcondition: complete() = true, all requests cleared (so no rerequests
			are done on timeout).
	add_block_remote:
		Add block that we know remote host has.
	add_host_complete:
		Add remote host that has all blocks.
	add_host_incomplete:
		Add remote host that has only some blocks.
	approve_block:
		Approve block for requesting. Blocks that aren't approved won't be
		requested.
	bytes:
		Returns size of bit_field. (bytes)
	complete:
		Returns true if we have all blocks.
	force_complete:
		Sets block request to indicate that we have all blocks.
	have_block:
		Returns true if we have specified block.
	next_request:
		Returns true and sets block to next block to request. Or returns false if
		host not yet added or no blocks to request from host.
		Precondition: !complete()
	remote_host_count:
		Returns the number of remote hosts block_request is keeping track of.
	*/
	void add_block_local(const boost::uint64_t block);
	void add_block_local(const int connection_ID, const boost::uint64_t block);
	void add_block_local_all();
	void add_block_remote(const int connection_ID, const boost::uint64_t block);
	void approve_block(const boost::uint64_t block);
	void approve_block_all();
	boost::uint64_t bytes();
	bool complete();
	bool have_block(const boost::uint64_t block);
	bool is_approved(const boost::uint64_t block);
	bool next_request(const int connection_ID, boost::uint64_t & block);
	unsigned remote_host_count();

	/* Incoming_Slot Related
	incoming_count:
		Number of hosts we're uploading file to.
	incoming_subscribe:
		Subscribes a Incoming_Slot to local bit_field updates. BF is set to our
		current local bit_field. The trigger_tick function is used to call the
		tick() function on the connection when there's a have_* message to send.
		This is called when constructing a Incoming_Slot.
	incoming_unsubscribe:
		Unsubscribes a Incoming_Slot to bit_field updates. This is called when
		a Incoming_Slot is removed.
		Postcondition: We will not keep track of updates and we will not call
			trigger for this connection_ID.
	next_have:
		Returns true and sets block to block we need to send in a have_* message.
		Returns false if no have messages to send.
	*/
	unsigned incoming_count();
	void incoming_subscribe(const int connection_ID,
		const boost::function<void(const int)> trigger_tick, bit_field & BF);
	void incoming_unsubscribe(const int connection_ID);
	bool next_have(const int connection_ID, boost::uint64_t & block);

	/* Outgoing_Slot Related
	outgoing_count:
		Number of hosts we're downloading file from.
	outgoing_subscribe:
		Add a bit_field of a remote host. If BF is empty it means the remote host
		has all blocks. This is called when a Outgoing_Slot is added.
	outgoing_unsubscribe:
		Remove the connection as a source of blocks. Cancel all unfulfilled
		requests. This is called when a Outgoing_Slot is removed.
	*/
	unsigned outgoing_count();
	void outgoing_subscribe(const int connection_ID, bit_field & BF);
	void outgoing_unsubscribe(const int connection_ID);

private:
	//locks access to all data members
	boost::mutex Mutex;

	//number of blocks (bits in the bit_fields)
	const boost::uint64_t block_count;

	/*
	Bits set to 0 represent blocks we need to request. Bits set to 1 represent
	blocks we have. If the container is empty it's the same as all bits being set
	to 1 (this is done to save memory).
	*/
	bit_field local;

	/*
	Bits set to 1 represent blocks we are allowed to request. Bits set to 0
	represent blocks we are forbidden from requesting. If the container is empty
	it means we are allowed to request any block.

	This is used for hash tree reconstruction to insure we only request blocks we
	can hash check.
	*/
	bit_field approved;

	/*
	Associates a block number with all the connections it has been requested
	from.
	std::map<block_num, std::list<connection_ID> >
	*/
	std::map<boost::uint64_t, std::set<int> > request;

	//connection_ID associated with bitset representing blocks remote host has
	std::map<int, bit_field> remote;

	class have_info
	{
	public:
		//have_* message blocks
		std::queue<boost::uint64_t> block;

		//triggers connection object to be called so it can process next_have
		boost::function<void(const int)> trigger_tick;
	};

	/*
	This container effectively contains diff's between the local bit_field when
	someone subscribed vs the local bit_field as it currently exists.
	*/
	std::map<int, have_info> have;

	/*
	find_next_rarest:
		Finds the next rarest block to request from the host with the specified
		connection_ID. This is called by next_request() after checking for timed out
		requests to re-request.
	*/
	bool find_next_rarest(const int connection_ID, boost::uint64_t & block);
};
#endif
