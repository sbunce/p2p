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

/*
There is no different between a requested block and a received block currently.
This will cause problems when we send someone our bit_field.

There has to be more info kept track of. We have to keep track of what blocks
were requested.

add_block_local should work normally, but next_request should not modify the
underlying bit_field, it should add the block number to the set.
*/

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
	*/
	void add_block_local(const int socket_FD, const boost::uint64_t block);
	void add_block_remote(const int socket_FD, const boost::uint64_t block);
	void add_host_complete(const int socket_FD);
	void add_host_incomplete(const int socket_FD, bit_field & BF);
	boost::uint64_t bytes();
	bool complete();
	bool next_request(const int socket_FD, boost::uint64_t & block);
	void remove_host(const int socket_FD);

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
	time. The socket_FD is needed so that we don't make a re-request to a host
	which 
	*/
	class request_element
	{
	public:
		request_element(
			const int socket_FD_in,
			const std::time_t request_time_in
		):
			socket_FD(socket_FD_in),
			request_time(request_time_in)
		{}

		const int socket_FD;            //socket request was made to
		const std::time_t request_time; //time request was made
	};
	std::multimap<boost::uint64_t, request_element> request;

	//socket_FD associated with bitset representing blocks remote host has
	std::map<int, bit_field> remote_block;

	/*
	find_next_rarest:
		Finds the next rarest block to request from the host with the specified
		socket_FD. This is called by next_request() after checking for timed out
		requests to re-request.
	*/
	bool find_next_rarest(const int socket_FD, boost::uint64_t & block);
};
#endif
