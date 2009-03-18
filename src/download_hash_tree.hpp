//NOT-THREADSAFE
#ifndef H_DOWNLOAD_HASH_TREE
#define H_DOWNLOAD_HASH_TREE

//boost
#include <boost/filesystem.hpp>

//custom
#include "block_arbiter.hpp"
#include "database.hpp"
#include "download.hpp"
#include "download_connection.hpp"
#include "download_info.hpp"
#include "hash_tree.hpp"
#include "protocol.hpp"
#include "request_generator.hpp"
#include "settings.hpp"

//include
#include <convert.hpp>
#include <sha.hpp>

//std
#include <vector>

class download_hash_tree : public download
{
public:
	download_hash_tree(const download_info & Download_Info_in);
	~download_hash_tree();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual download_info get_download_info();
	virtual const std::string hash();
	virtual const std::string name();
	virtual void pause();
	virtual unsigned percent_complete();
	virtual void remove();
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used);
	virtual bool response(const int & socket, std::string block);
	virtual void register_connection(const download_connection & DC);
	virtual const boost::uint64_t size();
	virtual void unregister_connection(const int & socket);

private:
	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	/*
	The tree_info for the file downloading.
	*/
	hash_tree::tree_info Tree_Info;

	/*
	When the file has finished downloading this will be set to true to indicate
	that P_CLOSE_SLOT commands need to be sent to all servers.
	*/
	bool close_slots;

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
			wait_activated(false),
			State(REQUEST_SLOT),
			slot_requested(false),
			slot_open(false)
		{}

		enum state{
			REQUEST_SLOT,   //need to request a slot
			REQUEST_BLOCKS, //slot received, requesting blocks
			PAUSED
		};
		state State;

		std::string IP;      //IP of server
		bool slot_requested; //true if slot has been requested
		bool slot_open;      //true if slot has been received
		char slot_ID;        //slot ID (set if slot_open = true)

		/*
		What file blocks have been requested from the server in order of when they
		were requested. When a block is received from the server the block number
		is latest_request.front().

		When new requests are made they should are pushed on to the back of
		latest_request.

		This is basically the pipeline.
		*/
		std::deque<boost::uint64_t> latest_request;

		/*
		If wait_activated is true then the server has sent a P_WAIT to indicate it
		doesn't yet have the block requested. The wait_start variable indicates
		when the server sent the P_WAIT. New requests of this server should not be
		made until settings::P_WAIT_TIMEOUT seconds have passed.
		*/
		bool wait_activated;
		time_t wait_start;
	};

	//socket number mapped to connection special pointer
	std::map<int, connection_special> Connection_Special;

	/*
	protocol_block:
		Handles an incoming hash block (message with P_BLOCK command).
	protocl_wait:
		Handles an incoming wait command that was sent in response to a block
		request.
	protocol_slot:
		Handles receiving response to slot request.
	*/
	void protocol_block(std::string & message, connection_special * conn);
	void protocol_wait(std::string & message, connection_special * conn);
	void protocol_slot(std::string & message, connection_special * conn);

	boost::shared_ptr<request_generator> Request_Generator;
	database::connection DB;
	download_info Download_Info;
	hash_tree Hash_Tree;
};
#endif
