//THREADSAFE
#ifndef H_HASH_TREE
#define H_HASH_TREE

//boost
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

//custom
#include "atomic_bool.hpp"
#include "convert.hpp"
#include "contiguous_map.hpp"
#include "database.hpp"
#include "global.hpp"
#include "request_generator.hpp"
#include "sha.hpp"

//std
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class hash_tree
{
public:
	hash_tree();

	//THREADSAFE
	class tree_info
	{
		friend class hash_tree;
	public:
		tree_info(
			const std::string & root_hash_in,
			const boost::uint64_t & file_size_in
		);

		/*
		The variables returned by const reference are guaranteed not to be modified after
		instantiation of the tree_info. This makes the const references thread safe.

		get_root_hash            - returns root hash of tree
		get_block_count          - returns number of hash blocks in tree
		get_tree_size            - returns size of tree in bytes
		get_file_block_count     - returns number of file blocks in file the tree is for
		get_file_size            - returns size of file the tree is for
		get_last_file_block_size - returns size of the last file block
		file_size_to_tree_size   - given the file size, returns the tree size
		*/
		const std::string & get_root_hash();
		const boost::uint64_t & get_block_count();
		const boost::uint64_t & get_tree_size();
		const boost::uint64_t & get_file_block_count();
		const boost::uint64_t & get_file_size();
		const boost::uint64_t & get_last_file_block_size();
		const std::deque<boost::uint64_t> & get_row();
		unsigned block_size(const boost::uint64_t & block_num);
		static boost::uint64_t file_size_to_tree_size(const boost::uint64_t & file_size);

		/*
		higest_good          - sets HG to highest good hash block in tree
		                       returns false if no good blocks exist in tree
		rerequest_bad_blocks - force_rerequests bad blocks, used by download_hash_tree
		block_info           - returns the size of the specified hash block
		*/
		bool highest_good(boost::uint64_t & HG);
		void rerequest_bad_blocks(request_generator & Request_Generator);

	private:
		//used by highest_good() and rerequest_bad_blocks()
		boost::shared_ptr<boost::recursive_mutex> Recursive_Mutex;

		/*************************************************************************
		NEED NOT BE LOCKED, BUT MUST NOT BE MODIFIED BY OUSIDE tree_info ctor
		*************************************************************************/
		database::blob Blob;                  //handle for hash tree
		std::string root_hash;                //root hash (hex)
		boost::uint64_t file_size;            //size of file hash tree is for
		boost::uint64_t block_count;          //hash block count
		boost::uint64_t tree_size;            //size of the hash tree (bytes)
		boost::uint64_t file_block_count;     //file block count
		boost::uint64_t last_file_block_size; //size of last file block
		boost::uint64_t file_hash_offset;     //offset (bytes) to start of file hashes
		std::deque<boost::uint64_t> row;      //number of hashes in each row
		/************************************************************************/

		/*************************************************************************
		ANY USE OF THESE MUST BE LOCKED WITH Recursive_Mutex
		*************************************************************************/
		/*
		The Contiguous container maps a hash block number to an IP address. When a
		bad block is detected all blocks from the server are removed from
		contiguous and added to bad_block.
		*/
		boost::shared_ptr<contiguous_map<boost::uint64_t, std::string> > Contiguous;
		std::vector<boost::uint64_t> bad_block;
		/************************************************************************/

		/*
		All these functionsa are pure. None require any locking.
		block_info - Requires a block number, and row info. Sets info.first to byte
		             offset to start of hash block and info.second to hash block
		             length. Parent is set to offset to start of parent hash.
		             Note: If block is 0 then parent won't be set.
		             Note: A version that doesn't set parent is available.
		file_size_to_file_hash  - Given file size (size of actual file, not hash tree),
		                          returns the number of file hashes (bottom row of tree).
		file_hash_to_tree_hash  - Given the file hash count (bottom row of tree), returns
		                          the number of hashes in the whole hash tree.
		                          Note: A second version that gives row info is available.
		file_size_to_tree_hash  - Given the file hash count (bottom row of tree), returns
		                          how many hashes are in the whole hash tree.
		row_to_block_count      - Given tree row information, returns hash block count
		                          (total number of hash blocks in tree).
		row_to_file_hash_offset - Returns the byte offset to the start of the file hashes.
		*/
		static bool block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
			std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent);
		static bool block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
			std::pair<boost::uint64_t, unsigned> & info);
		static boost::uint64_t file_size_to_file_hash(boost::uint64_t file_size);
		static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash_count);
		static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row);
		static boost::uint64_t file_size_to_tree_hash(const boost::uint64_t & file_size, std::deque<boost::uint64_t> & row);
		static boost::uint64_t row_to_block_count(const std::deque<boost::uint64_t> & row);
		static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row);
	};

	/*
	check            - checks the entire hash tree
	                   returns true if tree good, else false and sets bad_block to first bad block
	                   Note: must call this with the hash_tree_info before calling write_block
	check_file_block - checks a file block against a hash in the hash tree
	                   returns true if block good, else false
	create           - create hash tree
	                   returns true and sets root_hash if creation suceeded
	                   returns false if creation failed, or if stop called

	read_block       - get block from hash tree
	                   returns true if suceeded, else false if error opening file
	stop             - triggers early termination of create()
	write_block      - add block to hash tree, or replace block in hash tree
	                   returns true if block written, else false if writing error
	                   precondition: must have called check() with the hash_tree_info
	*/
	bool check(tree_info & Tree_Info, boost::uint64_t & bad_block);
	bool check_file_block(tree_info & Tree_Info, const boost::uint64_t & file_block_num, const char * block, const int & size);
	bool create(const std::string & file_path, std::string & root_hash);
	bool read_block(tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block);
	void stop();
	bool write_block(tree_info & Tree_Info, const boost::uint64_t & block_num, const std::string & block, const std::string & IP);

private:
	/*
	If a thread is in a long create_hash_tree call then this will cause it to
	terminate early and return false.
	*/
	atomic_bool stop_thread;

	/*
	check_block      - checks a hash tree block
	                   returns true if block good, else false if block bad
	                   precondition: the parent of this block must be valid an the block must exist
	check_contiguous - called after write_block() to check contiguous hash blocks blocks
	create_recurse   - recursively create hash tree, called by create()
	*/
	bool check_block(tree_info & Tree_Info, const boost::uint64_t & block_num, sha & SHA, char * block_buff);
	void check_contiguous(tree_info & Tree_Info);
	bool create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
		boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & root_hash, sha & SHA, char * block_buff);

	database::connection DB;
	database::table::blacklist DB_Blacklist;
};
#endif
