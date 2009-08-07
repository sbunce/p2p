#ifndef H_BLOCK_TRACK
#define H_BLOCK_TRACK

//include
#include <boost/dynamic_bitset.hpp>
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <limits>
#include <map>

class block_request
{
public:
	block_request(const boost::uint32_t block_count_in);

	/*
	add_block (1 parameter):
		Adds a block that we have that does not need to be requested. This is
		called when we hash check a file and discover that a block is good.
	add_block (2 parameters):
		Adds a block that a remote host has. This is called when a remote host
		tells us it got a new block.
	add_host (1 parameter):
		Adds a remote host that has all blocks available.
	add_host (2 parameters):
		Adds a bitset for a remote host. This function is used when the remote
		host sends us a bitfield because it doesn't have all blocks.
	complete:
		Returns true if all blocks have been requested.
	next:
		Returns true and sets block to the number of the next block we need to
		request from the remote host. If returns false then the remote host has no
		blocks we need.
		Precondition: !complete()
	remove_block:
		Schedule a block to be requested. This is called when we receive a block
		that hash fails, or if a server doesn't send us a block we requested.
	remove_host:
		Removes bitset for host. This is done when a host disconnects.
		Precondition: socket_FD element must exist in remote_block container.
	size:
		Returns the size of the bitset (bytes). This is how many bytes will be in
		an incoming bitfield from a remote host.
	*/
	void add_block(const boost::uint32_t block);
	void add_block(const int socket_FD, const boost::uint32_t block);
	void add_host(const int socket_FD);
	void add_host(const int socket_FD, boost::dynamic_bitset<> & BS);
	bool complete();
	bool next(const int socket_FD, boost::uint32_t & block);
	void remove_block(const boost::uint32_t block);
	void remove_host(const int socket_FD);
	boost::uint32_t size();

private:
	//number of blocks in the file
	boost::uint32_t block_count;

	//bits set to true need to be requested
	boost::dynamic_bitset<> local_block;

	//socket_FD associated with bitset representing blocks remote host has
	std::map<int, boost::dynamic_bitset<> > remote_block;
};
#endif
