#ifndef H_HASH_TREE
#define H_HASH_TREE

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//custom
#include "atomic_bool.h"
#include "convert.h"
#include "contiguous.h"
#include "DB_blacklist.h"
#include "global.h"
#include "request_generator.h"
#include "sha.h"

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

	/*
	There is a lot of calculation required to determine block offsets and lengths.
	By having this object that calculation only has to be done once.

	This object holds most of the state for needed for hash trees. The exception
	to this is the block buffer and SHA object which are large and can be reused.
	*/
	class tree_info
	{
		friend class hash_tree;
	public:
		tree_info(
			const std::string & root_hash_in,
			const boost::uint64_t file_size_in
		):
			root_hash(root_hash_in),
			file_size(file_size_in),
			highest_good(0)
		{
			file_size_to_tree_hash(file_size, row);
			block_count = row_to_block_count(row);
			file_hash_offset = row_to_file_hash_offset(row);
			tree_size = file_size_to_tree_size(file_size);

			Contiguous = NULL;
		}

		~tree_info()
		{
			if(Contiguous != NULL){
				delete Contiguous;
				Contiguous = NULL;
			}
		}

		const boost::uint64_t & get_block_count()
		{
			return block_count;
		}

		const boost::uint64_t & get_file_size()
		{
			return file_size;
		}

		const boost::uint64_t & get_tree_size()
		{
			return tree_size;
		}

		const boost::uint64_t & get_highest_good()
		{
			return highest_good;
		}

		/*
		Rerequests bad hash blocks.
		Note: this should only be used for when downloading hash trees.
		*/
		void rerequest_bad_blocks(request_generator & Request_Generator)
		{
			std::vector<boost::uint64_t>::iterator iter_cur, iter_end;
			iter_cur = bad_block.begin();
			iter_end = bad_block.end();
			while(iter_cur != iter_end){
				Request_Generator.force_rerequest(*iter_cur);
				++iter_cur;
			}
			bad_block.clear();
		}

	private:
		//minimum info needed to know everything about hash tree
		std::string root_hash;     //root hash (hex)
		boost::uint64_t file_size; //size of file hash tree is for

		//these are all calculated based on root_hash and file_size
		std::deque<boost::uint64_t> row;  //number of hashes in each row
		boost::uint64_t block_count;      //total hash block count
		boost::uint64_t file_hash_offset; //offset (bytes) to start of file hashes
		boost::uint64_t tree_size;        //size of the hash tree (bytes)

		/*
		Hash block mapped to IP. This is only needed if adding blocks to the hash
		tree. This is new'd by check() if there is a bad block in the tree.

		Note: If this is NULL when passed to write_block program will be terminated.
		*/
		contiguous<boost::uint64_t, std::string> * Contiguous;

		/*
		When a block is detected as being bad all blocks that were requested from
		the server that sent the bad block are put here to be rerequested.
		*/
		std::vector<boost::uint64_t> bad_block;

		//number of the highest good block (all blocks before this also good)
		boost::uint64_t highest_good;
	};

	/*
	check            - checks the entire hash tree
	                   returns true if tree good, else false and sets bad_block to first bad block
	                   Note: must call this with the tree_info before calling write_block
	check_file_block - checks a file block against a hash in the hash tree
	                   returns true if block good, else false
	create           - create hash tree
	                   returns true and sets root_hash if creation suceeded
	                   returns false if creation failed, or if stop called
	read_block       - get block from hash tree
	                   returns true if suceeded, else false if error opening file
	write_block      - add block to hash tree, or replace block in hash tree
	                   returns true if block written, else false if writing error
	                   precondition: must have called check() with the tree_info
	*/
	bool check(tree_info & Tree_Info, boost::uint64_t & bad_block);
	bool check_file_block(const tree_info & Tree_Info, const boost::uint64_t & file_block_num, const char * block, const int & size);
	bool create(const std::string & file_path, std::string & root_hash);
	bool read_block(const tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block);
	bool write_block(tree_info & Tree_Info, const boost::uint64_t & block_num, const std::string & block, const std::string & IP);

	/*
	If a thread is in the check() functions this will trigger it to terminate
	early and return false. The purpose of this is to force a long hashing
	process to terminate.
	*/
	void stop();

	/*
	Given a block number determines the size of the block (in bytes). Terminates
	the program if invalid block is specified.
	*/
	static unsigned int block_size(const tree_info & Tree_Info, const boost::uint64_t & block_num)
	{
		std::pair<boost::uint64_t, unsigned int> info;
		if(block_info(block_num, Tree_Info.row, info)){
			return info.second;
		}else{
			logger::debug(LOGGER_P1,"programmer error, invalid block specified");
			exit(1);
		}
	}

	/*
	Given the size of a file returns the size of the hash tree (bytes) that would
	be generated for the file.
	*/
	static boost::uint64_t file_size_to_tree_size(const boost::uint64_t & file_size)
	{
		return sha::HASH_SIZE * file_hash_to_tree_hash(file_size_to_file_hash(file_size));
	}

//private:

	/*
	Given a block number, sets info to the offset and length of the hash block.
	Returns true if valid block used as parameter, else false.
	info:   std::pair<offset(bytes), size(bytes)>
	parent: offset(bytes)

	Note: if block is 0 then paren't won't be set.
	*/
	static bool block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
		std::pair<boost::uint64_t, unsigned int> & info, boost::uint64_t & parent)
	{
		boost::uint64_t offset = 0;          //hash offset from beginning of file to start of row
		boost::uint64_t block_count = 0;     //total block count in all previous rows
		boost::uint64_t row_block_count = 0; //total block count in current row
		for(unsigned int x=0; x<row.size(); ++x){
			if(row[x] % global::HASH_BLOCK_SIZE == 0){
				row_block_count = row[x] / global::HASH_BLOCK_SIZE;
			}else{
				row_block_count = row[x] / global::HASH_BLOCK_SIZE + 1;
			}

			if(block_count + row_block_count > block){
				//end of row greater than block we're looking for, block exists in row
				info.first = offset + (block - block_count) * global::HASH_BLOCK_SIZE;

				//hashes between offset and end of current row
				boost::uint64_t delta = offset + row[x] - info.first;

				//determine size of block
				if(delta > global::HASH_BLOCK_SIZE){
					//full hash block
					info.second = global::HASH_BLOCK_SIZE;
				}else{
					//partial hash block
					info.second = delta;
				}

				//locate parent
				if(x != 0){
					//only set parent if not on first row (parent of first row is root hash)
					parent = offset - row[x-1]  //start of previous row
						+ block - block_count; //hash offset in to previous row is block offset in to current row
				}

				//convert to bytes
				info.first = info.first * sha::HASH_SIZE;
				info.second = info.second * sha::HASH_SIZE;
				parent = parent * sha::HASH_SIZE;

				return true;
			}

			block_count += row_block_count;
			offset += row[x];
		}
		return false;
	}

	/*
	Wrapper for above function. Used when parent is not needed.
	*/
	static boost::uint64_t throw_away;
	static bool block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
		std::pair<boost::uint64_t, unsigned int> & info)
	{
		return block_info(block, row, info, throw_away);
	}

	/*
	Given a file size (of original file, not hash tree), returns how many file
	hashes there would be (bottom row of the tree).
	*/
	static boost::uint64_t file_size_to_file_hash(boost::uint64_t file_size)
	{
		boost::uint64_t hash_count = file_size / global::FILE_BLOCK_SIZE;
		if(file_size % global::FILE_BLOCK_SIZE != 0){
			//add one for partial last block
			++hash_count;
		}
		return hash_count;
	}

	/*
	Given a number of file hashes, returns how many tree hashes there would be in
	the hash tree (total number of hashes).
	*/
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash_count)
	{
		boost::uint64_t start_hash = row_hash_count;
		if(row_hash_count == 1){
			//root hash not included in hash tree
			return 0;
		}else if(row_hash_count % global::HASH_BLOCK_SIZE == 0){
			row_hash_count = start_hash / global::HASH_BLOCK_SIZE;
		}else{
			row_hash_count = start_hash / global::HASH_BLOCK_SIZE + 1;
		}
		return start_hash + file_hash_to_tree_hash(row_hash_count);
	}

	/*
	Same as above but also populates deque with how many hashes are in earch row.
	*/
	static boost::uint64_t file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row)
	{
		boost::uint64_t start_hash = row_hash;
		if(row_hash == 1){
			//root hash not included in hash tree
			return 0;
		}else if(row_hash % global::HASH_BLOCK_SIZE == 0){
			row_hash = start_hash / global::HASH_BLOCK_SIZE;
		}else{
			row_hash = start_hash / global::HASH_BLOCK_SIZE + 1;
		}
		row.push_front(start_hash);
		return start_hash + file_hash_to_tree_hash(row_hash, row);
	}

	/*
	Given a file size (size of original file, not hash tree), returns how many
	tree hashes there would be. Also, populates deque with how many hashes in
	each row.
	*/
	static boost::uint64_t file_size_to_tree_hash(const boost::uint64_t & file_size, std::deque<boost::uint64_t> & row)
	{
		return file_hash_to_tree_hash(file_size_to_file_hash(file_size), row);
	}

	/*
	Given a deque or row hash counts, returns how many hash blocks there are.
	*/
	static boost::uint64_t row_to_block_count(const std::deque<boost::uint64_t> & row)
	{
		boost::uint64_t block_count = 0;
		for(int x=0; x<row.size(); ++x){
			if(row[x] % global::HASH_BLOCK_SIZE == 0){
				block_count += row[x] / global::HASH_BLOCK_SIZE;
			}else{
				block_count += row[x] / global::HASH_BLOCK_SIZE + 1;
			}
		}
		return block_count;
	}

	/*
	Given the row information returns the byte offset to the first file hash (bytes).
	*/
	static boost::uint64_t row_to_file_hash_offset(const std::deque<boost::uint64_t> & row)
	{
		boost::uint64_t file_hash_offset = 0;
		if(row.size() == 0){
			//no hash tree, root hash is file hash
			return file_hash_offset;
		}

		//add up the size (bytes) of all rows until reaching the beginning of the last row
		for(int x=0; x<row.size()-1; ++x){
			file_hash_offset += row[x] * sha::HASH_SIZE;
		}

		return file_hash_offset;
	}

	/*
	Buffer for both file blocks and file hash blocks.
	Note: A file block is always equal to the size of global::HASH_BLOCK_SIZE hashes.
	*/
	char block_buff[global::FILE_BLOCK_SIZE];

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
	bool check_block(const tree_info & Tree_Info, const boost::uint64_t & block_num);
	void check_contiguous(tree_info & Tree_Info);
	bool create_recurse(std::fstream & scratch, boost::uint64_t start_RRN,
		boost::uint64_t end_RRN, std::string & root_hash);

	sha SHA;
};
#endif
