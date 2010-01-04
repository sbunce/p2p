#ifndef H_TRANSFER
#define H_TRANSFER

//custom
#include "block_request.hpp"
#include "file.hpp"
#include "hash_tree.hpp"
#include "message.hpp"

class transfer
{
public:
	transfer(const file_info & FI);

	enum status{
		good,             //operation succeeded
		bad,              //operation failed
		protocol_violated //host violated protocol and should be blacklisted
	};

	/* Hash Tree
	next_request_hash_tree:
		Returns true and sets block_num, block_size, and tree_block_count if a
		hash tree block needs to be requested.
	read_tree_block:
		Reads block from hash tree and constructs block message. Returns good if
		block message created, bad if read failed, protocol_violated if tried to
		read a block we don't have.
	root_hash:
		Sets RH to root hash needed for slot message. Returns true if RH can be
		set, false if RH cannot be set.
	tree_block_count:
		Returns number of blocks in hash tree.
	write_hash_tree_block:
		Write block to hash tree.
	*/
	bool next_request_tree(const int connection_ID, boost::uint64_t & block_num,
		unsigned & block_size);
	status read_tree_block(boost::shared_ptr<message::base> & M,
		const boost::uint64_t block_num);
	bool root_hash(std::string & RH);
	boost::uint64_t tree_block_count();
	status write_tree_block(const int connection_ID, const boost::uint64_t block_num,
		const network::buffer & buf);

	/* File
	file_block_size:
		Returns number of blocks in file.
	file_size:
		Returns file size.
	next_request_file:
		Returns true and sets block_num, block-size, and file_block_count if a
		file block needs to be requested.
	read_file_block:
		Reads block from fiel and constructs block message. Returns good if block
		message created, bad if read failed, protocol_violated if tried to read a
		block we don't have.
	write_file_block:
		Hash checks file block and writes it to the file if it is good. Returns
		good if block good and written to file, bad if error writing to file,
		protocol_violated if block failed hash check.
	*/
	boost::uint64_t file_block_count();
	boost::uint64_t file_size();
	bool next_request_file(const int connection_ID, boost::uint64_t & block_num,
		unsigned & block_size);
	status read_file_block(boost::shared_ptr<message::base> & M,
		const boost::uint64_t block_num);
	status write_file_block(const int connection_ID, const boost::uint64_t block_num,
		const network::buffer & buf);

	/* Hash Tree + File
	check:
		Hash checks the hash tree and file. Called on program start on resumed
		transfers.
	complete:
		Returns true if hash tree and file are complete.
	get_status:
		Sets byte to status byte needed for slot message. Returns true if byte
		can be set, false if byte cannot be set.
	hash:
		Returns hash.
	register_outgoing_0:
		Register a host that has a complete hash tree and file.

	*/
	void check();
	bool complete();
	const std::string & hash();
	bool get_status(unsigned char & byte);
	void register_outgoing_0(const int connection_ID);

private:
	hash_tree Hash_Tree;
	file File;
	block_request Hash_Tree_Block;
	block_request File_Block;
};
#endif
