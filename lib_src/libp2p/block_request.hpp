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
	add_block_local:
		Adds a block that we have that does not need to be requested. This is
		called when we hash check a file and discover that a block is good.
	add_block_remote:
		Adds a block that a remote host has. This is called when a remote host
		tells us it got a new block.
	add_host_complete:
		Adds a remote host that has all blocks available.
	add_host_incomplete:
		Adds a bitset for a remote host. This function is used when the remote
		host sends us a bitfield because it doesn't have all blocks.
	append_bitfield:
		Appends our bitfield to buffer. The appended bitfield is big-endian.
	bytes:
		Returns the size of the bit_field (bytes).
	complete:
		Returns true if we have all blocks.
	next_request:
		Returns true and sets block to the number of the next block we need to
		request from the remote host. If returns false then the remote host has no
		blocks we need or we are waiting on some of the last blocks.
		Precondition: !complete()
	remove_host:
		Removes bit_field for host. This is done when a host disconnects or when
		we stop downloading the file from them. Any unfulfilled requests are
		immediately rerequested.
		Precondition: host must have been previously added.
	rerequest_block:
		This function is called when we receive a bad block but we don't know who
		it is from. When this happens we need to "re"request the block from any
		host. This function is commonly called when reassembling a hash tree.
	*/
	void add_block_local(const int connection_ID, const boost::uint64_t block);
	void add_block_remote(const int connection_ID, const boost::uint64_t block);
	void add_host_complete(const int connection_ID);
	void add_host_incomplete(const int connection_ID, bit_field & BF);
	boost::uint64_t bytes();
	bool complete();
	bool next_request(const int connection_ID, boost::uint64_t & block);
	void remove_host(const int connection_ID);
	void rerequest_block(const boost::uint64_t block);

private:
	//locks access to all data members
	boost::recursive_mutex Recursive_Mutex;

	//number of blocks (bits in the bit_fields)
	const boost::uint64_t block_count;

	/*
	Bits set to 1 represent blocks we need to request. Bits set to 0 represent
	blocks we have. If the container is empty it's the same as all bits being set
	to 0. This is done to save memory.
	*/
	bit_field local_block;

	/*
	This is used to rerequest blocks that haven't been received after a certain
	time. The connection_ID is needed so that we don't make a re-request to a host
	which 
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

		const int connection_ID;            //socket request was made to
		const std::time_t request_time; //time request was made
	};
	std::multimap<boost::uint64_t, request_element> request;

	//connection_ID associated with bitset representing blocks remote host has
	std::map<int, bit_field> remote_block;

	/*
	find_next_rarest:
		Finds the next rarest block to request from the host with the specified
		connection_ID. This is called by next_request() after checking for timed out
		requests to re-request.
	*/
	bool find_next_rarest(const int connection_ID, boost::uint64_t & block);
};
#endif
