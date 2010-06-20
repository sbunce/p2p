#ifndef H_TREE_INFO
#define H_TREE_INFO

//custom
#include "db_all.hpp"
#include "file_info.hpp"
#include "protocol_tcp.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class tree_info : private boost::noncopyable
{
public:
	tree_info(const file_info & FI);

	const std::string hash;                     //file tracked by this
	const boost::uint64_t file_size;            //size of file
	const std::deque<boost::uint64_t> row;      //hashes in each row (top is front())
	const boost::uint64_t tree_block_count;     //hash block count
	const boost::uint64_t file_hash_offset;     //offset (bytes) to start of file hashes
	const boost::uint64_t tree_size;            //size of the hash tree (bytes)
	const boost::uint64_t file_block_count;     //file block count (number of hashes in last row of tree)
	const boost::uint64_t last_file_block_size; //size of last file block

	/* Non-Static
	block_info (with parent paramter):
		Sets info to offset (bytes) and size of block. Sets parent to offset
		(bytes) of parent hash.
		Note: info.first is offset (bytes), info.second is size (bytes).
		Note: If block is 0 then parent won't be set.
	block_info (without parent parameter):
		Doesn't get parent.
	block_size:
		Returns the size of a hash block. This is used to know what to expect from
		a hash block request.
	file_block_children:
		Returns hash tree block children of specified hash tree block. Second
		element of pair set to true if block has children.
		std::pair<first child in range, one past last child in range>
	tree_block_children:
		Returns file block children of specified hash tree block. Second element
		of pair set to true if hash tree block has file block children.
		std::pair<first child in range, one past last child in range>
	*/
	bool block_info(const boost::uint64_t block,
		std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent) const;
	bool block_info(const boost::uint64_t block,
		std::pair<boost::uint64_t, unsigned> & info) const;
	unsigned block_size(const boost::uint64_t block_num) const;
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> file_block_children(
		const boost::uint64_t block) const;
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> tree_block_children(
		const boost::uint64_t block) const;

	/* Static
	file_size_to_tree_block_count:
		Given the file size, returns the tree block count. Used to get
		tree_block_count when hash_tree not instantiated.
	*/
	static boost::uint64_t calc_tree_block_count(boost::uint64_t file_size);

private:
	/*
	These functions are static to make it clear that none of them depend on
	non-static member variables. Most of these functions are called in the
	initializer list of the ctor.

	file_hash_to_tree_hash:
		Given the file hash count returns the number of hashes in tree. Populates
		row with how many hashes are in each row of the tree.
	file_size_to_file_hash:
		Given file size, returns number of file hashes (number of hashes in bottom
		row of tree).
	file_size_to_row:
		Given file_size, returns number of hashes in all rows. The front of the
		deque contains the highest (smallest) row in the tree.
	file_size_to_tree_size:
		Given file_size, returns tree size (bytes).
	row_to_file_hash_offset:
		Returns offset (bytes) to the start of file hashes.
	row_to_tree_block_count:
		Given tree row information, returns number of hash blocks.
	*/
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
	static boost::uint64_t file_size_to_file_hash(const boost::uint64_t file_size);
	static boost::uint64_t file_size_to_tree_size(const boost::uint64_t file_size);
	static std::deque<boost::uint64_t> file_size_to_row(const boost::uint64_t file_size);
	static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
	static boost::uint64_t row_to_tree_block_count(const std::deque<boost::uint64_t> & row);
};
#endif
