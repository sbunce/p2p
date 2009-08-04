/*
Thread safety of this class is like a file. You can read/write to different
parts concurrently but not the same parts.
*/
#ifndef H_HASH_TREE
#define H_HASH_TREE

//custom
#include "database.hpp"
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
		const std::string & root_hash_in,
		const boost::uint64_t & file_size_in
	);

	enum status{
		GOOD,    //block is good
		BAD,     //block is bad
		IO_ERROR //error reading hash tree
	};

	/*
	The order of these is important since they're used in the ctor initializer
	list and a lot of them have dependencies.
	*/
	const std::string root_hash;                //root hash (hex)
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
	check_file_block:
		Checks a file block against a hash in the hash tree. Returns GOOD if file
		block good. Returns BAD if file block bad. Returns IO_ERROR if cannot read
		hash tree.
		Precondition: The complete function must return true.
	complete:
		Returns true if the hash tree is complete. Complete means that all hashes
		have been checked good. If this function returns false then the
		missing_block function can be used to get the number of a block that needs
		to be requested.
	create:
		Create a hash tree. Returns true and sets root_hash if creation suceeded.
		The expected file size should be set to the size the file is when create()
		is called. If the file changes size while hashing the create function will
		return false.
		Note: If hash tree creation suceeds the hash tree state will only be set
			to RESERVED. If it's not changed the tree will be deleted upon next
			program start.
	file_size_to_tree_size:
		Given a file_size, returns tree size (bytes).
	missing_block:
		Returns a missing_block number.
		Precondition: The complete function must return false.
	read_block:
		Get block from hash tree. Returns GOOD if suceeded. Returns IO_ERROR if
		cannot read hash tree.
	write_block:
		Add block to hash tree, or replace block in hash tree. Returns GOOD if
		block written sucessfully. Returns IO_ERROR if could not write block.
		Precondition: Must have called check() to make sure the block is good.
	*/
	unsigned block_size(const boost::uint64_t & block_num);
	status check_file_block(const boost::uint64_t & file_block_num,
		const char * block, const int & size);
	static bool create(const std::string & file_path,
		const boost::uint64_t & expected_file_size, std::string & root_hash);
	bool complete();
	static boost::uint64_t file_size_to_tree_size(const boost::uint64_t & file_size);
	boost::uint64_t missing_block();
	status read_block(const boost::uint64_t & block_num, std::string & block);
	status write_block(const boost::uint64_t & block_num, const std::string & block,
		const std::string & IP);

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
	The block_status container holds bitgroups of size 1. When the bit is 0 the
	block is not good, when the bit is 1 the block has been written but it's not
	necessarily good. Only if the block is < end_of_good is the hash block
	good.

	Hashes inherently have to be checked in order. If a block is missing we can't
	check blocks past it. The end_of_good value is how far we've been able to
	hash check.

	We don't keep track of what IP sent what block so we only have access to the
	IP of the latest block that arrived. We are still able to blacklist people
	that send bad blocks because if someone is sending bad blocks eventually they
	will send a block equal to the value of end_of_good and get blacklisted.
	*/
	bitgroup_vector block_status;
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
	status check_incremental(boost::uint64_t & bad_block);
	static bool create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
		boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & root_hash,
		char * block_buff, SHA1 & SHA);
	static boost::uint64_t file_size_to_file_hash(boost::uint64_t file_size);
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash_count);
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
	static std::deque<boost::uint64_t> file_size_to_row(const boost::uint64_t & file_size);
	static boost::uint64_t row_to_tree_block_count(const std::deque<boost::uint64_t> & row);
	static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
};
#endif
