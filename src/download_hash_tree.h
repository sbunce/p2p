#ifndef H_DOWNLOAD_HASH_TREE
#define H_DOWNLOAD_HASH_TREE

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//custom
#include "convert.h"
#include "client_server_bridge.h"
#include "database.h"
#include "download.h"
#include "download_connection.h"
#include "download_info.h"
#include "global.h"
#include "hash_tree.h"
#include "request_generator.h"
#include "sha.h"

//std
#include <vector>

class download_hash_tree : public download
{
public:
	download_hash_tree(const download_info & Download_Info_in);
	~download_hash_tree();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string hash();
	virtual const std::string name();
	virtual unsigned percent_complete();
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used);
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
	const download_info & get_download_info();

private:
	/*
	If the hash tree download gets cancelled early by the user this will be set
	to true which will make the completion of the hash tree not trigger a
	download_file to start.

	Downloads cancelled by a user immediately become invisible because they might
	take a little while to clean up.
	*/
	bool _cancel;
	bool _visible;

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
			State(REQUEST_SLOT)
		{

		}

		enum state{
			REQUEST_SLOT,   //need to request a slot
			AWAITING_SLOT,  //slot requested, awaiting slot
			REQUEST_BLOCKS, //slot received, requesting blocks
			CLOSED_SLOT     //already sent a P_CLOSE
		};
		state State;

		std::string IP; //IP of server
		char slot_ID;   //slot ID the server gave for the file

		/*
		What file blocks have been requested from the server in order of when they
		were requested. When a block is received from the server the block number
		is latest_request.front().

		When new requests are made they should are pushed on to the back of
		latest_request.

		This is basically the pipeline for the server.
		*/
		std::deque<boost::uint64_t> latest_request;

		/*
		If wait_activated is true then the server has sent a P_WAIT to indicate it
		doesn't yet have the block requested. The wait_start variable indicates
		when the server sent the P_WAIT. New requests of this server should not be
		made until global::P_WAIT_TIMEOUT seconds have passed.
		*/
		bool wait_activated;
		time_t wait_start;
	};

	//socket number mapped to connection special pointer
	std::map<int, connection_special> Connection_Special;

	database::connection DB;
	database::table::hash DB_Hash;
	database::table::download DB_Download;
	download_info Download_Info;
	request_generator Request_Generator;
	hash_tree Hash_Tree;
};
#endif
