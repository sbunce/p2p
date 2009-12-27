#ifndef H_TRANSFER
#define H_TRANSFER

//custom
#include "file.hpp"
#include "hash_tree.hpp"

class transfer
{
public:
	transfer(const file_info & FI);

	/* Hash Tree
	root_hash:
		Sets RH to root hash needed for slot message. Returns true if RH can be
		set, false if RH cannot be set.
	write_hash_tree_block:
		Writes block to hash tree.
	*/
	bool root_hash(std::string & RH);
	hash_tree::status write_hash_tree_block(const boost::uint64_t block_num,
		const network::buffer & block);

	/* File
	file_size:
		Returns file size.
	*/
	boost::uint64_t file_size();

	/* Hash Tree + File
	check:
		Hash checks the hash tree and file. Called on program start on resumed
		transfers.
	complete:
		Returns true if hash tree and file are complete.
	hash:
		Returns hash.
	status:
		Sets byte to status byte needed for slot message. Returns true if byte
		can be set, false if byte cannot be set.
	*/
	void check();
	bool complete();
	const std::string & hash();
	bool status(unsigned char & byte);

private:
	file File;
	hash_tree Hash_Tree;
};
#endif
