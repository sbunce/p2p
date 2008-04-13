#ifndef H_HASH_TREE
#define H_HASH_TREE

//std
#include <string>

//custom
#include "sha.h"
#include "global.h"

class hash_tree
{
public:
	hash_tree();

	/*
	check_exists            - returns true if hash tree file exists (note: this doesn't mean the tree is corrrect)
	check_hash_tree         - returns true if bad hash or missing hash found (bad_hash set to possible bad hashes)
	                          returns false if no bad hash found in the hash tree	                          
	create_hash_tree        - creates a hash tree for the file pointed to by file_path
	                          returns the root node hash
	file_size_to_hash_count - returns how many hashes there would be for a file of size file_size
	replace_hash            - used to replace bad hashes found by check_hash_tree
	*/
	bool check_exists(std::string root_hash);
	bool check_hash_tree(std::string root_hash, unsigned long hash_count, std::pair<unsigned long, unsigned long> & bad_hash);
	std::string create_hash_tree(std::string file_path);
	void replace_hash(std::string root_hash, const unsigned long & number, char hash[]);

	//returns how many file hashes there would be for a file of size file_size
	static unsigned int file_size_to_hash_count(unsigned long file_size)
	{
		unsigned int hashes = file_size / hash_length();

		//add one hash for a partial last file block
		if(file_size % hash_length() != 0){
			++hashes;
		}
		return hashes;
	}

	//returns how many hashes there would be in a hash tree of size file_size
	static unsigned long file_size_to_hash_tree_count(unsigned long file_size)
	{
		unsigned long file_hashes = file_size / (global::P_BLOCK_SIZE - 1);

		//there may be a partial last block which must also be hashed
		if(file_size % (global::P_BLOCK_SIZE - 1) != 0){
			++file_hashes;
		}

		unsigned long row_hashes = file_hashes;
		unsigned long total_hashes = 0;

		//when one hash is reached only the root hash remains
		while(row_hashes != 1){
			total_hashes += row_hashes;
			unsigned long prev_row_hashes = row_hashes;
			row_hashes = row_hashes / 2;

			//add a hash if there is an odd amount of hashes in the row
			if(prev_row_hashes % 2 != 0){
				++row_hashes;
			}
		}

		//add in the root hash
		++total_hashes;

		return total_hashes;
	}

	//wraps the sha::hash_length() function so classes that use hash_tree don't have to know about sha
	static int hash_length()
	{
		return sha::hash_length(global::HASH_TYPE);
	}

private:
	sha SHA;

	int hash_len;   //how many bytes the raw hash is
	int block_size; //how many bytes in one file block

	/*
	Used by check_tree to maintain the current position in the tree. Time is saved
	by not rechecking already verified parts of the tree.
	*/
	unsigned long current_RRN; //RRN of latest hash retrieved
	unsigned long start_RRN;   //RRN of the start of the current row
	unsigned long end_RRN;     //RRN of the end of the current row

	//stores the root node of the latest root hash used with check_hash_tree
	std::string check_tree_latest;

	//stops create_hash_tree_recurse from writing if existing hash tree found
	bool create_hash_tree_recurse_found;

	/*
	check_hash               - returns true if the hash of the child hashes match the parent hash
	create_hash_tree_recurse - called by create_hash_tree to recursively create hash tree rows past the first
	locate_start             - returns the RRN of the start of the file hashes
	*/
	inline bool check_hash(char parent[], char left_child[], char right_child[]);
	void create_hash_tree_recurse(std::fstream & scratch, std::streampos row_start, std::streampos row_end, std::string & root_hash);
	unsigned long locate_start(std::string root_hash);
};
#endif
