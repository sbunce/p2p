#ifndef H_TRANSFER
#define H_TRANSFER

//custom
#include "block_request.hpp"
#include "file.hpp"
#include "hash_tree.hpp"
#include "message_tcp.hpp"
#include "peer.hpp"
#include "protocol_tcp.hpp"

//include
#include <atomic_int.hpp>
#include <net/net.hpp>

class transfer : private boost::noncopyable
{
public:
	transfer(const file_info & FI);

	enum status{
		good,             //operation succeeded
		bad,              //operation failed
		protocol_violated //host violated protocol and should be blacklisted
	};

	class next_request
	{
	public:
		next_request(
			const boost::uint64_t block_num_in,
			const unsigned block_size_in
		);
		next_request(const next_request & NR);
		boost::uint64_t block_num;
		unsigned block_size;
	};

	class local_BF
	{
	public:
		bit_field tree_BF;
		bit_field file_BF;
	};

	/* Hash Tree
	next_have_file:
		Returns info to be sent in have_hash_tree_block message.
	next_request_hash_tree:
		Returns info to be send in request_hash_tree_block message.
	read_tree_block:
		Read a block from the hash tree.
		Note: The message is only valid if status = good.
	recv_have_hash_tree_block:
		Called when we get a have_hash_tree_block message.
	root_hash:
		Returns the root hash of the hash tree.
	tree_block_count:
		Returns number of blocks in hash tree.
	tree_percent_complete:
		Returns % of the tree we have (0-100).
	tree_size:
		Returns size of hash tree (bytes).
	write_hash_tree_block:
		Write block to hash tree.
	*/
	boost::optional<boost::uint64_t> next_have_tree(const int connection_ID);
	boost::optional<next_request> next_request_tree(const int connection_ID);
	std::pair<boost::shared_ptr<message_tcp::send::base>, status> read_tree_block(
		const boost::uint64_t block_num);
	void recv_have_hash_tree_block(const int connection_ID, const boost::uint64_t block_num);
	boost::optional<std::string> root_hash();
	boost::uint64_t tree_block_count();
	unsigned tree_percent_complete();
	boost::uint64_t tree_size();
	status write_tree_block(const int connection_ID, const boost::uint64_t block_num,
		const net::buffer & buf);

	/* File
	file_block_size:
		Returns number of blocks in file.
	file_percent_complete:
		Returns % of file we have (0-100).
	file_size:
		Returns file size.
	next_have_file:
		Returns info to be send in request_file_block message.
	next_request_file:
		Returns info to be send in request_file_block message.
	read_file_block:
		Read a block from file.
		Note: The message is only valid if status = good.
	recv_have_file_block:
		Called when we get a have_file_block message.
	write_file_block:
		Write block to file.
		Note: This function hash checks the block.
	*/
	boost::uint64_t file_block_count();
	unsigned file_percent_complete();
	boost::uint64_t file_size();
	boost::optional<boost::uint64_t> next_have_file(const int connection_ID);
	boost::optional<next_request> next_request_file(const int connection_ID);
	std::pair<boost::shared_ptr<message_tcp::send::base>, status> read_file_block(
		const boost::uint64_t block_num);
	void recv_have_file_block(const int connection_ID, const boost::uint64_t block_num);
	status write_file_block(const int connection_ID, const boost::uint64_t block_num,
		const net::buffer & buf);

	/* Hash Tree + File
	check:
		Hash checks the hash tree and file. Called on program start on resumed
		transfers.
	complete:
		Returns true if hash tree and file are complete.
	download_subscribe:
		Called when we recv slot message to add remote host bit_fields.
	download_unsubscribe:
		Called when outgoing slot removed.
	next_peer:
		Returns endpoint that needs to be sent in peer_* message.
	percent_complete:
		Returns percent complete of the hash tree and file combined.
	upload_subscribe:
		Subscribe to changes to hash tree and file.
	upload_unsubscribe:
		Unsubscribe to changes to hash tree and file.
	*/
	void check();
	bool complete();
	void download_subscribe(const int connection_ID, const net::endpoint & ep,
		const bit_field & tree_BF, const bit_field & file_BF);
	void download_unsubscribe(const int connection_ID);
	boost::optional<net::endpoint> next_peer(const int connection_ID);
	unsigned percent_complete();
	local_BF upload_subscribe(const int connection_ID, const net::endpoint & ep,
		const boost::function<void()> trigger_tick);
	void upload_unsubscribe(const int connection_ID);

	/* Speeds
	download_speed:
		Returns download speed (bytes/second).
	download_speed_calc:
		Returns shared_ptr to speed calculator responsible for download speed.
	upload_speed:
		Returns upload speed (bytes/second).
	*/
	unsigned download_speed();
	const boost::shared_ptr<net::speed_calc> & download_speed_calc();
	unsigned upload_speed();

	/* Connection Counts
	download_count:
		Number of hosts we're downloading file from.
	upload_count:
		Number of hosts we're uploading file to.
	*/
	unsigned download_count();
	unsigned upload_count();

private:
	hash_tree Hash_Tree;
	file File;
	block_request Tree_Block;
	block_request File_Block;
	peer Peer;

	//number of bytes received (used to calculate percent complete)
	atomic_int<boost::uint64_t> bytes_received;

	boost::shared_ptr<net::speed_calc> Download_Speed;
	boost::shared_ptr<net::speed_calc> Upload_Speed;
};
#endif
