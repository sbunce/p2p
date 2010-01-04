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

	//info provided by file_info
	const std::string hash;                     //hash the file is tracked by
	const std::string path;                     //path to file the hash tree is for
	const boost::uint64_t file_size;            //size of file hash tree is for
	const database::blob blob;                  //blob handle for hash tree in database

	//info generated based on file_info
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
		Checks the entire hash tree. This is called on program start to see what
		blocks are good.
	check_file_block:
		Checks a file block against a hash in the hash tree. Returns good if file
		block good. Returns bad if file block bad. Returns io_error if cannot read
		hash tree.
		Precondition: The complete function must return true.
	create:
		Create hash tree. Returns good if tree created or already existed. Returns
		bad if tree could never be created. Returns io_error if an error occured
		reading the tree or temp files.
		Note: If tree created the tree state will be set to reserved. This must
			be set to complete or tree will be deleted on next program start.
		Note: The create function modifies const data members.
	file_block_children:
		Returns hash tree block children of specified hash tree block. Second
		element of pair set to true if block has children.
		std::pair<first child in range, one past last child in range>
	read_block:
		Get block from hash tree. Returns good if suceeded (and block appended to
		buf). Returns io_error if cannot read hash tree.
	tree_block_children:
		Returns file block children of specified hash tree block. Second element
		of pair set to true if hash tree block has file block children.
		std::pair<first child in range, one past last child in range>
	write_block:
		Add or replace block in hash tree. Returns good if block was written,
		io_error if block could not be written, or bad if the block hash failed.
		The connection_ID is needed for block_request.
		Precondition: Parent hash must have been written.
		Note: This function does not check validity of root_hash (block 0).
	*/
	unsigned block_size(const boost::uint64_t block_num);
	status check();
	status check_file_block(const boost::uint64_t file_block_num,
		const network::buffer & buf);
	status create();
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> file_block_children(
		const boost::uint64_t block);
	status read_block(const boost::uint64_t block_num, network::buffer & buf);
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> tree_block_children(
		const boost::uint64_t block);
	status write_block(const boost::uint64_t block_num,
		const network::buffer & buf);

private:
	/*
	block_info (with parent paramter):
		Sets info to offset (bytes) and size of block. Sets parent to offset
		(bytes) of parent hash.
		Note: info.first is offset (bytes), info.second is size (bytes).
		Note: If block is 0 then parent won't be set.
	block_info (without parent parameter):
		Doesn't get parent.
	check_block:
		Checks a hash tree block that already exists in tree. This reads both the
		parent and child in to memory and hash checks.
		Precondition: Parent and child must exist in tree.
	*/
	bool block_info(const boost::uint64_t block,
		std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent);
	bool block_info(const boost::uint64_t block,
		std::pair<boost::uint64_t, unsigned> & info);
	status check_block(const boost::uint64_t block_num);

	/*
	These functions are static to make it clear that none of them depend on
	non-static member variables. Most of these functions are called in the
	initializer list of the ctor.

	file_size_to_file_hash:
		Given file size, returns number of file hashes (number of hashes in bottom
		row of tree).
	file_size_to_tree_size:
		Given file_size, returns tree size (bytes).
	file_hash_to_tree_hash:
		Given the file hash count returns the number of hashes in tree. Populates
		row with how many hashes are in each row of the tree.
	row_to_block_count:
		Given tree row information, returns number of hash blocks.
	row_to_file_hash_offset:
		Returns offset (bytes) to the start of file hashes.
	*/
	static boost::uint64_t file_size_to_file_hash(const boost::uint64_t file_size);
	static boost::uint64_t file_size_to_tree_size(const boost::uint64_t file_size);
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
	static std::deque<boost::uint64_t> file_size_to_row(const boost::uint64_t file_size);
	static boost::uint64_t row_to_tree_block_count(const std::deque<boost::uint64_t> & row);
	static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
};
#endif
