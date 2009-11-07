/*
The ctor will throw an exception if database access fails. The thread safety of
this class is exactly like a file.
*/
#ifndef H_HASH_TREE
#define H_HASH_TREE

//custom
#include "block_request.hpp"
#include "database.hpp"
#include "file_info.hpp"
#include "path.hpp"
#include "protocol.hpp"
#include "settings.hpp"

//include
#include <atomic_int.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <convert.hpp>
#include <network/network.hpp>
#include <SHA1.hpp>

//standard
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class hash_tree : private boost::noncopyable
{
public:
	/*
	If FI.hash is empty then create must be called. Until then then the hash_tree
	won't be connected to any blob in the database.
	*/
	hash_tree(
		const file_info & FI,
		database::pool::proxy DB = database::pool::proxy()
	);

	enum status{
		good,     //block is good
		bad,      //block is bad
		io_error, //error reading hash tree
		copying   //file may be copying (it is increasing in size)
	};

	const std::string hash;                     //hash the file is tracked by
	const std::string path;                     //path to file the hash tree is for
	const boost::uint64_t file_size;            //size of file hash tree is for
	const std::deque<boost::uint64_t> row;      //number of hashes in each row
	const boost::uint64_t tree_block_count;     //hash block count
	const boost::uint64_t file_hash_offset;     //offset (bytes) to start of file hashes
	const boost::uint64_t tree_size;            //size of the hash tree (bytes)
	const boost::uint64_t file_block_count;     //file block count (number of hashes in last row of tree)
	const boost::uint64_t last_file_block_size; //size of last file block

	/*
	block_size:
		Returns the size of a hash block. This is used to know what to expect from
		a hash block request.
	check:
		If the tree is not complete the check function should be called to
		determine what blocks are good.
		Note: This function does a lot of disk access and can take a long time to
			run.
	check_file_block:
		Checks a file block against a hash in the hash tree. Returns GOOD if file
		block good. Returns BAD if file block bad. Returns IO_ERROR if cannot read
		hash tree.
		Precondition: The complete function must return true.
	complete:
		Returns true if the hash tree is complete. Complete means that all hashes
		have been checked good. If this funtion returns false then the missing
		block to request should be gotten from Block_Request. It should be noted
		that Block_Request can be complete and the hash_tree not complete if the
		request for the last blocks were made, but not yet received.
	create:
		Create a hash tree. Returns good if tree created or tree already existed.
		Returns bad if hash tree could never be created. Returns io_error if
		io_error occured (such as file changing size, temp files failing to read,
		etc). When an io_error occurs the file is generally retried later.
		Precondition: FI.path and FI.file_size must be correctly set.
		Postcondition: FI.hash set unless io_error returned.
		Note: If hash tree creation suceeds the hash tree state will only be set
			to RESERVED. If it's not changed the tree will be deleted upon next
			program start.
	read_block:
		Get block from hash tree. Returns GOOD if suceeded. Returns IO_ERROR if
		cannot read hash tree.
	write_block:
		Add or replace block in hash tree. Returns good if block was written,
		io_error if block could not be written, or bad if the block hash failed.
		The socket_FD is needed for block_request.
	*/
	unsigned block_size(const boost::uint64_t & block_num);
	status check();
	status check_file_block(const boost::uint64_t & file_block_num,
		const char * block, const int & size);
	status create();
	bool complete();
	status read_block(const boost::uint64_t & block_num, std::string & block);
	status write_block(const int socket_FD, const boost::uint64_t & block_num,
		const std::string & block);

	/*
	This is used to determine what blocks to request when downloading the hash
	tree. The check functions will call block_request::remove_block when a block
	hash fails.
	Note: The block_request keeps track of all the blocks we have, not blocks
		which are known to be good. For a block to be good it must be in
		Block_Request and the block number must be < end_of_good.
	*/
	block_request Block_Request;

private:
	//blob handle for hash tree
	const database::blob Blob;

	/*
	Hashes are checked in order. If a block is missing we can't check blocks past
	it. The end_of_good value is how far we've been able to hash check.

	We don't keep track of what IP sent what block so we only have access to the
	IP of the latest block that arrived. We are still able to blacklist people
	that send bad blocks because if someone is sending bad blocks eventually they
	will send a block equal to the value of end_of_good and get blacklisted.
	*/
	atomic_int<boost::uint64_t> end_of_good;

	/*
	check_block:
		Checks a hash tree block.
		Precondition: the parent of this block must be valid and the block must
			exist.
		Note: This function is only called from check_incremental which makes sure
		the precondition is satisfied.
	*/
	status check_block(const boost::uint64_t & block_num);

	/*
	These functions are static to make it clear that none of them depend on
	non-static member variables. Most of these functions are called in the
	initializer list of the ctor.

	block_info:
		Requires a block number, and row info. Sets info.first to byte offset
		to start of hash block and info.second to hash block length. Parent is
		set to offset of start of parent hash.
		Note: If block is 0 then parent won't be set.
		Note: A version that doesn't set parent is available.
	file_size_to_file_hash:
		Given file size (size of actual file, not hash tree), returns the
		number of file hashes (bottom row of tree).
	file_size_to_tree_size:
		Given a file_size, returns tree size (bytes).
	file_hash_to_tree_hash:
		Given the file hash count returns the number of hashes in tree. Populates
		row with how many hashes are in each row of the tree.
	row_to_block_count:
		Given tree row information, returns hash block count (total number of
		hash blocks in tree).
	row_to_file_hash_offset:
		Returns the byte offset to the start of the file hashes.
	*/
	static bool block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
		std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent);
	static bool block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
		std::pair<boost::uint64_t, unsigned> & info);
	static boost::uint64_t file_size_to_file_hash(const boost::uint64_t & file_size);
	static boost::uint64_t file_size_to_tree_size(const boost::uint64_t & file_size);
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
	static std::deque<boost::uint64_t> file_size_to_row(const boost::uint64_t & file_size);
	static boost::uint64_t row_to_tree_block_count(const std::deque<boost::uint64_t> & row);
	static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
};
#endif
