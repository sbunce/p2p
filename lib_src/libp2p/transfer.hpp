#ifndef H_TRANSFER
#define H_TRANSFER

//custom
#include "block_request.hpp"
#include "file.hpp"
#include "hash_tree.hpp"
#include "message.hpp"

//include
#include <atomic_int.hpp>
#include <network/network.hpp>

class transfer : private boost::noncopyable
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
	hash:
		Returns hash.
	get_status:
		Sets byte to status byte needed for slot message. Returns true if byte
		can be set, false if byte cannot be set.
	percent_complete:
		Returns percent complete of the hash tree and file combined.
	register_outgoing_0:
		Register a host that has a complete hash tree and file.

	*/
	void check();
	bool complete();
	bool get_status(unsigned char & byte);
	const std::string & hash();
	unsigned percent_complete();
	void register_outgoing_0(const int connection_ID);

	/* Speeds
	download_speed:
		Returns download speed (bytes/second).
	touch:
		Add 0 bytes to Download_Speed/Upload_Speed to force recalculation.
	upload_speed:
		Returns upload speed (bytes/second).
	*/
	unsigned download_speed();
	void touch();
	unsigned upload_speed();

private:
	hash_tree Hash_Tree;
	file File;
	block_request Hash_Tree_Block;
	block_request File_Block;

	//number of bytes received (used to calculate percent complete)
	atomic_int<boost::uint64_t> bytes_received;

	network::speed_calculator Download_Speed;
	network::speed_calculator Upload_Speed;
};
#endif
