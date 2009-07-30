//NOT-THREADSAFE
#ifndef H_HASH_TREE
#define H_HASH_TREE

//custom
#include "database.hpp"
//#include "request_generator.hpp"
#include "path.hpp"
#include "protocol.hpp"
#include "settings.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <convert.hpp>
#include <contiguous_map.hpp>
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
	hash_tree();

	class tree_info : private boost::noncopyable
	{
		friend class hash_tree;
	public:
		tree_info(
			const std::string & root_hash_in,
			const boost::uint64_t & file_size_in
		);

//DEBUG, can these be replaced with const variables?
		//see documentation for private variables returned
		const std::string & get_root_hash();
		const boost::uint64_t & get_tree_size();
		const boost::uint64_t & get_file_size();
		const boost::uint64_t & get_tree_block_count();
		const boost::uint64_t & get_file_block_count();
		const boost::uint64_t & get_last_file_block_size();

		/*
		block_size:
			Returns the size of a hash block.
		higest_good:
			Sets HG to highest good hash block in tree. Returns false if no good
			blocks exist in tree.
		rerequest_bad_blocks:
			Force rerequest bad blocks (used by download_hash_tree).
		block_info:
			Returns the size of the specified hash block.
		file_size_to_tree_size:
			Given a file size, returns how large the hash tree would be (bytes).
		*/
		unsigned block_size(const boost::uint64_t & block_num);
		bool highest_good(boost::uint64_t & HG);
		//boost::uint64_t rerequest_bad_blocks(request_generator & Request_Generator);
		static boost::uint64_t file_size_to_tree_size(const boost::uint64_t & file_size);

	private:
		database::blob Blob;                  //handle for hash tree
		std::string root_hash;                //root hash (hex)
		boost::uint64_t tree_size;            //size of the hash tree (bytes)
		boost::uint64_t file_size;            //size of file hash tree is for
		boost::uint64_t tree_block_count;     //hash block count
		boost::uint64_t file_block_count;     //file block count
		boost::uint64_t last_file_block_size; //size of last file block
		boost::uint64_t file_hash_offset;     //offset (bytes) to start of file hashes
		std::deque<boost::uint64_t> row;      //number of hashes in each row

		/*
		The Contiguous container maps a hash block number to an IP address. When a
		bad block is detected all blocks from the server are removed from
		contiguous and added to bad_block.
		*/
		contiguous_map<boost::uint64_t, std::string> Contiguous;
		std::vector<boost::uint64_t> bad_block;

		/* All pure functions, none require locking.
		block_info:
			Requires a block number, and row info. Sets info.first to byte offset
			to start of hash block and info.second to hash block length. Parent is
			set to offset to start of parent hash.
			Note: If block is 0 then parent won't be set.
			Note: A version that doesn't set parent is available.
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
		static boost::uint64_t file_size_to_file_hash(boost::uint64_t file_size);
		static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash_count);
		static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
		static boost::uint64_t file_size_to_tree_hash(const boost::uint64_t & file_size, std::deque<boost::uint64_t> & row);
		static boost::uint64_t row_to_tree_block_count(const std::deque<boost::uint64_t> & row);
		static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
	};

	enum status{
		GOOD,    //block is good
		BAD,     //block is bad
		IO_ERROR //error reading hash tree
	};

	/*
	check:
		Checks the entire hash tree. Returns GOOD if whole tree good. Returns BAD
		and sets bad_block to first bad block if tree bad. Returns IO_ERROR if
		error reading tree.
		Note: must call this with the hash_tree_info before calling write_block
	check_file_block:
		Checks a file block against a hash in the hash tree. Returns GOOD if file
		block good. Returns BAD if file block bad. Returns IO_ERROR if cannot read
		hash tree.
	create:
		Create hash tree. Returns true and sets root_hash if creation suceeded.
		Returns false if creation failed, error reading tree, if stop called, or
		if thread interrupted. The expected_file_size is passed to this function
		so that hashing can be cancelled if the file is growing (which saves
		time on file that will be rehashed anyways).
	get_tree_info:
		Returns tree info needed to read and write hash tree.
		Note: This is an expensive function to call. The returned hash tree should
		      be saved and reused.
	read_block:
		Get block from hash tree. Returns GOOD if suceeded. Returns IO_ERROR if
		cannot read hash tree.
	stop:
		Triggers early termination of create().
	write_block:
		Add block to hash tree, or replace block in hash tree. Returns GOOD if
		block written sucessfully. Returns IO_ERROR if could not write block.
		Precondition: Must have called check() to make sure the block is good.
	*/
	status check(tree_info & Tree_Info, boost::uint64_t & bad_block);
	status check_file_block(tree_info & Tree_Info, const boost::uint64_t & file_block_num, const char * block, const int & size);
	bool create(const std::string & file_path, const boost::uint64_t & expected_file_size, std::string & root_hash);
	status read_block(tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block);
	void stop();
	status write_block(tree_info & Tree_Info, const boost::uint64_t & block_num, const std::string & block, const std::string & IP);

private:
	/*
	If a thread is in a long create_hash_tree call then this will cause it to
	terminate early and return false.
	*/
	atomic_bool stop_thread;

	//buffer used for hash blocks
	char block_buff[protocol::FILE_BLOCK_SIZE];

	/*
	check_block:
		Checks a hash tree block.
		Precondition: the parent of this block must be valid and the block must exist.
	check_contiguous - called after write_block() to check contiguous hash blocks blocks
	create_recurse   - recursively create hash tree, called by create()
	*/
	status check_block(tree_info & Tree_Info, const boost::uint64_t & block_num);
	void check_contiguous(tree_info & Tree_Info);
	bool create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
		boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & root_hash);

	SHA1 SHA;
};
#endif
