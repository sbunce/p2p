#ifndef H_TRANSFER
#define H_TRANSFER

//custom
#include "file.hpp"
#include "hash_tree.hpp"
#include "message.hpp"

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
	register_outgoing_0:
		Register a host that has a complete hash tree and file.
	request:
		Returns a request_hash_tree_block or request_file_block message. Returns
		empty shared_ptr if no requests to make.
	status:
		Sets byte to status byte needed for slot message. Returns true if byte
		can be set, false if byte cannot be set.
	*/
	void check();
	bool complete();
	const std::string & hash();
	void register_outgoing_0(const int connection_ID);
	boost::shared_ptr<message::base> request(const int connection_ID,
		const unsigned char slot_num);
	bool status(unsigned char & byte);

private:
	hash_tree Hash_Tree;
	file File;
	block_request Hash_Tree_Block;
	block_request File_Block;
};
#endif
