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
	percent_complete:
		Returns % of blocks we have (0-100).
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
	unsigned percent_complete();

	/* Upload
	upload_count:
		Number of hosts we're uploading file to.
	next_have:
		Returns block that needs to be sent in have_* message.
	upload_subscribe:
		Subscribes a Incoming_Slot to local bit_field updates. The local bit_field
		is returned. The trigger_tick function is used to call the tick() function
		on the connection when there's a have_* message to send. This is called
		when constructing a Incoming_Slot.
	upload_unsubscribe:
		Unsubscribes a Incoming_Slot to bit_field updates. This is called when
		a Incoming_Slot is removed.
		Postcondition: We will not keep track of updates and we will not call
			trigger for this connection_ID.
	*/
	unsigned upload_count();
	boost::optional<boost::uint64_t> next_have(const int connection_ID);
	bit_field upload_subscribe(const int connection_ID,
		const boost::function<void()> trigger_tick);
	void upload_unsubscribe(const int connection_ID);

	/* Download
	download_count:
		Number of hosts we're downloading file from.
	download_subscribe:
		Add a bit_field of a remote host. If BF is empty it means the remote host
		has all blocks. This is called when a Outgoing_Slot is added.
	download_unsubscribe:
		Remove the connection as a source of blocks. Cancel all unfulfilled
		requests. This is called when a Outgoing_Slot is removed.
	*/
	unsigned download_count();
	void download_subscribe(const int connection_ID, const bit_field & BF);
	void download_unsubscribe(const int connection_ID);

private:
	//object set up as a monitor, lock used for all public functions
	boost::mutex Mutex;

	//number of blocks (bits in the bit_fields)
	const boost::uint64_t block_count;

	//number of blocks we have
	boost::uint64_t local_blocks;

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

	//block number associated with connection_ID requested from
	std::map<boost::uint64_t, std::set<int> > Request;

	class download_element
	{
	public:
		download_element(const bit_field & BF);
		download_element(const download_element & DE);

		//remote bit_field, when we receive have_* messages we update this
		bit_field block_BF;
	};
	//key is connection_ID
	std::map<int, download_element> Download;

	class upload_element
	{
	public:
		upload_element(const boost::function<void()> & trigger_tick_in);
		upload_element(const upload_element & UE);

		//trigger tick on connection this object represents
		boost::function<void()> trigger_tick;

		//diff between original local bit_field state and current state
		std::queue<boost::uint64_t> block_diff;
	};
	//key is connection_ID
	std::map<int, upload_element> Upload;

	/*
	find_next_rarest:
		Returns next rarest block we need to request.
	*/
	boost::optional<boost::uint64_t> find_next_rarest(const int connection_ID);
};
#endif
