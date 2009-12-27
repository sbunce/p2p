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
	remove_host:
		Remove host as source for blocks. Any blocks requested from this host are
		rerequested.
	unfulfilled:
		Returns the number of unfulfilled requests.
	*/
	void add_block_local(const boost::uint64_t block);
	void add_block_local(const int connection_ID, const boost::uint64_t block);
	void add_block_local_all();
	void add_block_remote(const int connection_ID, const boost::uint64_t block);
	void add_host_complete(const int connection_ID);
	void add_host_incomplete(const int connection_ID, bit_field & BF);
	void approve_block(const boost::uint64_t block);
	void approve_block_all();
	boost::uint64_t bytes();
	bool complete();
	bool have_block(const boost::uint64_t block);
	bool is_approved(const boost::uint64_t block);
	bool next_request(const int connection_ID, boost::uint64_t & block);
	void remove_host(const int connection_ID);
	unsigned unfulfilled(const int connection_ID);

private:
	//locks access to all data members
	boost::recursive_mutex Recursive_Mutex;

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
	This is used to rerequest blocks that haven't been received after a certain
	time. The connection_ID is needed so that we don't make a rerequest to a
	host which has a timed out block request.
	*/
	class request_element
	{
	public:
		request_element(
			const int connection_ID_in,
			const std::time_t request_time_in
		):
			connection_ID(connection_ID_in),
			request_time(request_time_in)
		{}
		request_element(const request_element & RE):
			connection_ID(RE.connection_ID),
			request_time(RE.request_time)
		{}
		const int connection_ID;        //socket request was made to
		const std::time_t request_time; //time request was made
	};
	std::multimap<boost::uint64_t, request_element> request;

	//connection_ID associated with bitset representing blocks remote host has
	std::map<int, bit_field> remote;

	/*
	find_next_rarest:
		Finds the next rarest block to request from the host with the specified
		connection_ID. This is called by next_request() after checking for timed out
		requests to re-request.
	*/
	bool find_next_rarest(const int connection_ID, boost::uint64_t & block);
};
#endif
