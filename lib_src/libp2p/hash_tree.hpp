/*
The ctor will throw an exception if database access fails.
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
#include <atomic_bool.hpp>
#include <bitgroup_vector.hpp>
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
	hash_tree(
		const file_info & FI,
		database::pool::proxy DB = database::pool::proxy()
	);

	enum status{
		good,    //block is good
		bad,     //block is bad
		io_error //error reading hash tree
	};

	/*
	The order of these is important since they're used in the ctor initializer
	list and a lot of them have dependencies.
	*/
	const std::string hash;                     //root hash (hex)
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
	file_size_to_tree_size:
		Given a file_size, returns tree size (bytes).
	read_block:
		Get block from hash tree. Returns GOOD if suceeded. Returns IO_ERROR if
		cannot read hash tree.
	write_block:
		Add block to hash tree, or replace block in hash tree. Returns GOOD if
		block written sucessfully. Returns IO_ERROR if could not write block.
		Precondition: Must have called check() to make sure the block is good.
	*/
	unsigned block_size(const boost::uint64_t & block_num);
	void check();
	status check_file_block(const boost::uint64_t & file_block_num,
		const char * block, const int & size);
	static status create(file_info & FI);
	bool complete();
	static boost::uint64_t file_size_to_tree_size(const boost::uint64_t & file_size);
	status read_block(const boost::uint64_t & block_num, std::string & block);
	status write_block(const boost::uint64_t & block_num, const std::string & block,
		const std::string & IP);

	/*
	This is used to determine what blocks to request when downloading the hash
	tree. The check functions will call block_request::remove_block when a block
	hash fails.
	*/
//DEBUG, consider not having this public.
//This messes up the thread safety guarantee.
	block_request Block_Request;

private:
	//blob handle for hash tree
	database::blob Blob;

	/*
	If the ctor has to allocate space for a hash tree this bool will be set to
	true to indicate that the write_block function needs to set the state of the
	hash tree to downloading.
	Note: A reference to the hash tree must exist somewhere in the database
		before the write_block function is called otherwise there can be a
		"memory leak" (unused but allocated space in the database).
	*/
	bool set_state_downloading;

	/*
	When the bit is 0 the block is not good, when the bit is 1 the block has been
	written but it's not necessarily good. Only if the block is < end_of_good is
	the hash block good.

	Hashes inherently have to be checked in order. If a block is missing we can't
	check blocks past it. The end_of_good value is how far we've been able to
	hash check.

	We don't keep track of what IP sent what block so we only have access to the
	IP of the latest block that arrived. We are still able to blacklist people
	that send bad blocks because if someone is sending bad blocks eventually they
	will send a block equal to the value of end_of_good and get blacklisted.
	*/
	boost::dynamic_bitset<> block_status;
	boost::uint64_t end_of_good;

	/*
	block_info:
		Requires a block number, and row info. Sets info.first to byte offset
		to start of hash block and info.second to hash block length. Parent is
		set to offset to start of parent hash.
		Note: If block is 0 then parent won't be set.
		Note: A version that doesn't set parent is available.
	check_block:
		Checks a hash tree block.
		Precondition: the parent of this block must be valid and the block must
			exist.
		Note: This function only called from check_incremental and check_full
			which both make sure the precondition is satisfied.
	check_incremental:
		Checks received blocks (blocks where block_status[block_num] != 0) that
		are contiguous (there is no block_status <= highest_good that is equal
		to zero). Returns GOOD if whole tree good. Returns BAD and sets bad_block
		to first bad block if tree bad. Returns IO_ERROR if error reading tree.
		Postcondition: highest_good is set to the highest block number that hash
			checked good.
		Postcondition: If returns BAD then sets block_status of the bad block to
			zero.
	create_recurse:
		Recursive function called by create() to recursively generate tree rows.
	file_size_to_file_hash:
		Given file size (size of actual file, not hash tree), returns the
		number of file hashes (bottom row of tree).
	file_hash_to_tree_hash:
		Given the file hash count (bottom row of tree), returns the number of
		hashes in the whole hash tree.
		Note: A second version that gives row info is available.
	file_size_to_tree_hash:
		Given the file hash count (bottom row of tree), returns how many hashes
		are in the whole hash tree.
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
	status check_block(const boost::uint64_t & block_num);
	status check_incremental(boost::uint32_t & bad_block);
	static bool create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
		boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & hash,
		char * block_buff, SHA1 & SHA);
	static boost::uint64_t file_size_to_file_hash(boost::uint64_t file_size);
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash_count);
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
	static std::deque<boost::uint64_t> file_size_to_row(const boost::uint64_t & file_size);
	static boost::uint64_t row_to_tree_block_count(const std::deque<boost::uint64_t> & row);
	static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
};
#endif
