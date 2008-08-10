#ifndef H_DOWNLOAD_HASH_TREE
#define H_DOWNLOAD_HASH_TREE

//custom
#include "convert.h"
#include "DB_blacklist.h"
#include "download.h"
#include "download_connection.h"
#include "global.h"
#include "hash_tree.h"
#include "request_gen.h"
#include "sha.h"

//std
#include <vector>

class download_hash_tree : public download
{
public:
	download_hash_tree(
		const std::string & root_hash_in,
		const boost::uint64_t & download_file_size_in,
		const std::string & download_file_name_in
	);

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string hash();
	virtual const std::string name();
	virtual unsigned int percent_complete();
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual void register_connection(const download_connection & DC);
	virtual const boost::uint64_t size();
	virtual void unregister_connection(const int & socket);
	virtual bool visible();

	/*
	canceled           - returns true if download canceled, stops download_file from being triggered
	download_file_size - returns the size of the file the hash tree was generated for
	download_file_name - returns the name of the file the hash tree was generated for
	*/
	const bool & canceled();
	const boost::uint64_t & download_file_size();
	const std::string & download_file_name();

private:
	//this information is for the file the hash tree was generated for
	boost::uint64_t _download_file_size;
	std::string _download_file_name;

	/*
	If the hash tree download gets cancelled early by the user this will be set
	to true which will make the completion of the hash tree not trigger a
	download_file to start.
	*/
	bool _canceled;

	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	std::string root_hash;     //root hash of the tree downloading
	std::string hash_name;     //the name of this hash
	boost::uint64_t hash_tree_count;  //number of hashes in the tree
	boost::uint64_t hash_block_count; //number of hash blocks
	boost::uint64_t hashes_per_block; //number of hashes in a hash block

	/*
	When the file has finished downloading this will be set to true to indicate
	that P_CLOSE_SLOT commands need to be sent to all servers.
	*/
	bool close_slots;

	/*
	When all hashes are done downloading the checking phase is entered where if
	there are any bad blocks they're requested one at a time from low to high.
	*/
	bool checking_phase;

	/*
	Class for storing information specific to servers. Each server is identified
	by it's socket number which is unique.
	*/
	class connection_special
	{
	public:
		connection_special(
			const std::string IP_in
		):
			IP(IP_in),
			slot_ID_requested(false),
			slot_ID_received(false),
			close_slot_sent(false),
			abusive(false)
		{

		}

		std::string IP;         //IP of server
		char slot_ID;           //slot ID the server gave for the file
		bool slot_ID_requested; //true if slot_ID requested
		bool slot_ID_received;  //true if slot_ID received

		/*
		When the download hash tree is entirely downloaded the download goes in to
		the checking phase where the hash tree is checked. If this server sent a
		bad block it will be blacklisted and abusive will be set to true. When
		abusive is set to true
		*/
		bool abusive;

		/*
		The download will not be finished until a P_CLOSE_SLOT has been sent to all
		servers.
		*/
		bool close_slot_sent;

		/*
		What file blocks have been requested from the server in order of when they
		were requested. When a block is received from the server the block number
		is latest_request.front().

		When new requests are made they should be pushed on to the back of
		latest_request.
		*/
		std::deque<boost::uint64_t> latest_request;

		/*
		This stores all the blocks that have been received from the server. When the
		every block is downloaded there will be a hash check. If any block from this
		server fails a hash check all blocks later than the bad block requested from
		this server will be re_requested.

		The purpose of re_requesting before checking is to maximize the speed of the
		download. In order to re_request sequentially the block would have be be
		re_requested from one server and all others would have to wait until it was
		known that the replacement block was good.
		*/
		std::set<boost::uint64_t> requested_blocks;
	};

	//socket number mapped to connection special pointer
	std::map<int, connection_special> Connection_Special;

	request_gen Request_Gen;
	hash_tree Hash_Tree;
};
#endif
