#ifndef H_TRANSFER
#define H_TRANSFER

//custom
#include "block_request.hpp"
#include "file.hpp"
#include "hash_tree.hpp"
#include "message_tcp.hpp"
#include "peer.hpp"
#include "protocol_tcp.hpp"
#include "speed_composite.hpp"

//include
#include <atomic_int.hpp>
#include <net/net.hpp>
#include <p2p.hpp>

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
	std::pair<net::buffer, status> read_tree_block(const boost::uint64_t block_num);
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
	std::pair<net::buffer, status> read_file_block(const boost::uint64_t block_num);
	void recv_have_file_block(const int connection_ID, const boost::uint64_t block_num);
	status write_file_block(const int connection_ID, const boost::uint64_t block_num,
		const net::buffer & buf);

	/* Hash Tree + File
	check:
		Hash checks the hash tree and file. Called on program start on resumed
		transfers.
	complete:
		Returns true if hash tree and file are complete.
	download_reg:
		Register a download. Called when we recv a slot message.
	download_unreg:
		Called when outgoing slot removed.
	next_peer:
		Returns endpoint that needs to be sent in peer_* message.
	percent_complete:
		Returns percent complete of the hash tree and file combined.
	upload_reg:
		Subscribe to changes to hash tree and file.
	upload_unreg:
		Unsubscribe to changes to hash tree and file.
	*/
	void check();
	bool complete();
	void download_reg(const int connection_ID, const net::endpoint & ep,
		const bit_field & tree_BF, const bit_field & file_BF);
	void download_unreg(const int connection_ID);
	boost::optional<net::endpoint> next_peer(const int connection_ID);
	unsigned percent_complete();
	local_BF upload_reg(const int connection_ID, const net::endpoint & ep,
		const boost::function<void()> trigger_tick);
	void upload_unreg(const int connection_ID);

	/* Other
	download_hosts:
		Number of hosts we're downloading file from.
	download_speed:
		Returns download speed (bytes/second).
	download_speed_composite:
		Returns download speed calculator.
	host_info:
		Returns info for all connected hosts.
	upload_hosts:
		Number of hosts we're uploading file to.
	upload_speed:
		Returns upload speed (bytes/second).
	upload_speed_composite:
		Returns upload speed calculator.
	*/
	unsigned download_hosts();
	unsigned download_speed();
	speed_composite download_speed_composite(const int connection_ID);
	std::list<p2p::transfer_info::host_element> host_info();
	unsigned upload_hosts();
	unsigned upload_speed();
	speed_composite upload_speed_composite(const int connection_ID);

private:
	hash_tree Hash_Tree;
	file File;
	block_request Tree_Block;
	block_request File_Block;
	peer Peer;

	//total hash_tree and file bytes received (doesn't include protocol overhead)
	atomic_int<boost::uint64_t> bytes_received;

	//total speeds
	boost::shared_ptr<net::speed_calc> Download_Speed;
	boost::shared_ptr<net::speed_calc> Upload_Speed;
};
#endif
