#ifndef H_HASH_TREE
#define H_HASH_TREE

//std
#include <string>

#include "sha.h"
#include "global.h"

class hash_tree
{
public:
	hash_tree();

	/*
	check_hash_tree  - returns true if a bad pair of hashes found (stored in bad_hash)
	                   this function should NEVER be called until a full hash tree exists
	create_hash_tree - creates a hash tree for the file pointed to by file_path
	                   returns the root node hash
	replace_hash     - used to replace bad hashes found by check_hash_tree
	*/
	bool check_hash_tree(std::string root_hash, unsigned long & hash_count, std::pair<unsigned long, unsigned long> & bad_hash);
	std::string create_hash_tree(std::string file_path);
	void replace_hash(std::fstream & hash_fstream, const unsigned long & number, char hash[]);

private:
	sha SHA;

	//holds how many hashes were in the last generated tree
	unsigned long hash_count;

	int hash_length; //how many bytes the raw hash is
	int block_size;  //how many bytes in one file block

	/*
	Used by check_tree to maintain the current position in the tree. Time is saved
	by not rechecking already verified parts of the tree.
	*/
	unsigned long current_RRN; //RRN of latest hash retrieved
	unsigned long start_RRN;   //RRN of the start of the current row
	unsigned long end_RRN;     //RRN of the end of the current row

	//stores the root node of the latest root hash used with check_hash_tree
	std::string check_tree_latest;

	/*
	check_hash               - returns true if the hash of the child hashes match the parent hash
	create_hash_tree_recurse - called by create_hash_tree to recursively create hash tree rows past the first
	locate_start             - returns the RRN of the start of the file hashes
	*/
	bool check_hash(char parent[], char left_child[], char right_child[]);
	int create_hash_tree_recurse(std::fstream & scratch, std::streampos row_start, std::streampos row_end, std::string & root_hash);
	unsigned long locate_start(std::string root_hash);
};
#endif
