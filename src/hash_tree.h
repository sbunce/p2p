#ifndef H_HASH_TREE
#define H_HASH_TREE

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>

//custom
#include "convert.h"
#include "global.h"
#include "sha.h"

//std
#include <fstream>
#include <iostream>
#include <string>

class hash_tree
{
public:
	hash_tree();

	/*
	check_block             - checks a file block to see if it matches the block in the hash tree
	                          returns true if block valid, else false
	check_hash_tree         - returns true if bad hash or missing hash found (bad_hash set to possible bad hashes)
	                          returns false if no bad hash found in the hash tree	                          
	create_hash_tree        - creates a hash tree for the file pointed to by file_path
	                          returns true and sets root_hash if tree created
	                          calling stop() while tree generating can trigger return of false
	file_hash               - returns the file hash that corresponds to root_hash_hex and block_number
	file_size_to_hash_count - returns how many hashes there would be for a file of size file_size
	write_hash              - used to replace bad hashes found by check_hash_tree
	stop                    - sets stop_thread to true and allows create hash tree to exit early
	*/
	bool check_block(const std::string & root_hex_hash, const boost::uint64_t & block_number, const char * const block, const int & block_length);
	bool check_hash_tree(const std::string & root_hash, const boost::uint64_t & hash_count, std::pair<boost::uint64_t, boost::uint64_t> & bad_hash);
	bool create_hash_tree(std::string file_path, std::string & root_hash);
	void write_hash(const std::string & root_hex_hash, const boost::uint64_t & number, const std::string & hash_block);
	void stop();

	//returns how many file hashes there would be for a file of size file_size
	static boost::uint64_t file_hash_count(boost::uint64_t file_size)
	{
		boost::uint64_t hash_count = file_size / global::FILE_BLOCK_SIZE;
		if(file_size % global::FILE_BLOCK_SIZE != 0){
			//add one for partial last block
			++hash_count;
		}
		return hash_count;
	}

	//returns how many hashes there would be in a hash tree of size file_size
	static boost::uint64_t hash_tree_count(boost::uint64_t row_hash)
	{
		boost::uint64_t start_hash = row_hash;
		if(row_hash == 1){
			return 0;
		}else if(row_hash % 2 != 0){
			++start_hash;
			row_hash = start_hash / 2;
		}else{
			row_hash = start_hash / 2;
		}
		return start_hash + hash_tree_count(row_hash);
	}

private:
	//mutex for all access to public functions except stop()
	boost::mutex Mutex;

	/*
	If a thread is in a long create_hash_tree call then this will cause it to
	terminate early.
	*/
	bool stop_thread;

	/*
	Used by check_tree() to maintain the current position in the tree. Time is saved
	by not rechecking already verified parts of the tree.
	*/
	boost::uint64_t current_RRN; //RRN of latest hash retrieved
	boost::uint64_t start_RRN;   //RRN of the start of the current row
	boost::uint64_t end_RRN;     //RRN of the end of the current row

	/*
	Used by file_hash() to store the last root hash and start RRN it was told
	to get a block for. Time is saved by not locating the start of the block
	hashes every time file_hash() is called.
	*/
	std::string file_hash_root_hex_hash;
	boost::uint64_t file_hash_start_RRN;
	char file_hash_buffer[sha::HASH_LENGTH];

	//stores the root node of the latest root hash used with check_hash_tree
	std::string check_tree_latest;

	/*
	check_hash               - returns true if the hash of the child hashes match the parent hash
	create_hash_tree_recurse - called by create_hash_tree to recursively create hash tree rows past the first
	locate_start             - returns the RRN of the start of the file hashes
	*/
	bool check_hash(const char * parent, char * left_child, char * right_child);
	void create_hash_tree_recurse(std::fstream & scratch, std::streampos row_start, std::streampos row_end, std::string & root_hash);
	boost::uint64_t locate_start(const std::string & root_hex_hash);

	sha SHA;
};
#endif
