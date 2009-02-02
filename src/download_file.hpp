//NOT-THREADSAFE

#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

//custom
#include "atomic_int.hpp"
#include "convert.hpp"
#include "client_server_bridge.hpp"
#include "database.hpp"
#include "download.hpp"
#include "download_info.hpp"
#include "global.hpp"
#include "hash_tree.hpp"
#include "request_generator.hpp"

//std
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

class download_file : public download
{
public:
	download_file(const download_info & Download_Info_in);

	~download_file();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual download_info get_download_info();
	virtual const std::string hash();
	virtual const std::string name();
	virtual unsigned percent_complete();
	virtual void register_connection(const download_connection & DC);
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const boost::uint64_t size();
	virtual void unregister_connection(const int & socket);

private:

	boost::thread hashing_thread;     //thread to check file
	bool hashing;                     //true if hashing
	atomic_int<int> hashing_percent;  //percent done hashing
	boost::uint64_t first_unreceived; //block at end of file upon download start + 1

	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	boost::uint64_t block_count; //number of file blocks (last block is block_count - 1)
	unsigned last_block_size;    //holds the exact size of the last fileBlock(in bytes)

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

		std::string IP;         //IP of server
		char slot_ID;           //slot ID the server gave for the file

		/*
		What file blocks have been requested from the server in order of when they
		were requested. When a block is received from the server the block number
		is latest_request.front().

		When new requests are made they should be pushed on to the back of
		latest_request.
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

	/*
	hash_check  - checks partial download integrity (run in thread spawned in ctor)
	write_block - writes a file block
	*/
	void hash_check(hash_tree::tree_info Tree_Info, std::string file_path);
	void write_block(boost::uint64_t block_number, std::string & block);

	download_info Download_Info;
	database::table::blacklist DB_Blacklist;
	request_generator Request_Generator;
	hash_tree Hash_Tree;
};
#endif
