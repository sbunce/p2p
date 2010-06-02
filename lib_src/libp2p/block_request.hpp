#ifndef H_BLOCK_REQUEST
#define H_BLOCK_REQUEST

//include
#include <bit_field.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <logger.hpp>
#include <net/net.hpp>

//standard
#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <set>

/*
The block_request class keeps track of what blocks we have and what blocks
remote hosts have. It also can determine the rarest blocks to request.
*/
class block_request : private boost::noncopyable
{
public:
	block_request(const boost::uint64_t block_count_in);

	/*
	add_block_local (one paramter):
		Add block. Used when next_request was not involved in getting a block.
		This function is used during hash check to mark what blocks we already
		have.
		Precondition: No hosts are subscribed.
	add_block_local (two parameters):
		Add block from specific host.
	add_block_local_all:
		Adds all local blocks.
		Postcondition: complete() = true, all requests cleared.
	add_block_remote:
		Add block that we know remote host has.
	approve_block:
		Approve block for requesting. Blocks that aren't approved won't be
		requested.
	approve_block_all:
		Approve all blocks for requesting.
	bytes:
		Returns size of bit_field. (bytes)
	complete:
		Returns true if we have all blocks.
	have_block:
		Returns true if we have specified block.
	is_approved:
		Returns true if the block has been approved to be requested.
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
	boost::optional<boost::uint64_t> next_request(const int connection_ID);
	unsigned remote_host_count();

	/* Incoming Slot (slots the remote host has opened with us)
	incoming_count:
		Number of hosts we're uploading file to.
	incoming_subscribe:
		Subscribes a Incoming_Slot to local bit_field updates. The local bit_field
		is returned. The trigger_tick function is used to call the tick() function
		on the connection when there's a have_* message to send. This is called
		when constructing a Incoming_Slot.
	incoming_unsubscribe:
		Unsubscribes a Incoming_Slot to bit_field updates. This is called when
		a Incoming_Slot is removed.
		Postcondition: We will not keep track of updates and we will not call
			trigger for this connection_ID.
	next_have:
		Returns block that needs to be sent in have_* message.
	*/
	unsigned incoming_count();
	bit_field incoming_subscribe(const int connection_ID,
		const boost::function<void()> trigger_tick);
	void incoming_unsubscribe(const int connection_ID);
	boost::optional<boost::uint64_t> next_have(const int connection_ID);

	/* Outgoing Slot (slots we have opened with the remote host)
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
	void outgoing_subscribe(const int connection_ID, const bit_field & BF);
	void outgoing_unsubscribe(const int connection_ID);

private:
	//object set up as a monitor, lock used for all public functions
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
	Associates a block number with all the connections it has been requested from.
	std::map<block_num, std::list<connection_ID> >
	*/
	std::map<boost::uint64_t, std::set<int> > request;

	//connection_ID associated with bit_field representing blocks remote host has
	std::map<int, bit_field> remote;

	class have_element
	{
	public:
		have_element(const boost::function<void()> & trigger_tick_in);
		have_element(const have_element & HE);
		/*
		Calling this will cause the connection::tick() function to be called for
		the connection this object is for. This is called after blocks are added.
		*/
		boost::function<void()> trigger_tick;
		/*
		This is a block diff between the local blocks when the host subscribed and
		the present. These block numbers are sent in have_* messages.
		*/
		std::queue<boost::uint64_t> block;
	};
	std::map<int, have_element> Have;

	/*
	find_next_rarest:
		Returns next rarest block we need to request.
	*/
	boost::optional<boost::uint64_t> find_next_rarest(const int connection_ID);
};
#endif
