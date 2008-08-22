#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

//boost
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

//custom
#include "convert.h"
#include "DB_blacklist.h"
#include "download.h"
#include "global.h"
#include "hash_tree.h"
#include "request_generator.h"

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
	download_file(
		const std::string & file_hash_in,
		const std::string & file_name_in, 
		const std::string & file_path_in,
		const boost::uint64_t & file_size_in
	);

	~download_file();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string hash();
	virtual const std::string name();
	virtual unsigned int percent_complete();
	virtual void register_connection(const download_connection & DC);
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const boost::uint64_t size();
	virtual void unregister_connection(const int & socket);
	virtual bool visible();

private:
	/*
	When any writing is happening to the downloading file all read access should
	be blocked.
	*/
	boost::mutex file_mutex;

	/*
	When the download_file is constructed a thread is spawned to hash check a
	partial download that has been resumed. Requests are only returned when hashing
	is done which is indicated by this being set equal to false.
	*/
	volatile bool hashing;

	/*
	Percent done hashing the partial file when download resumed.
	*/
	volatile int hashing_percent;

	/*
	This is set to the first unreceived block (next block past end of file).
	*/
	boost::uint64_t first_unreceived;

	volatile int threads;       //one if hash checking, otherwise zero
	volatile bool stop_threads; //if set to true hash checking will stop early

	/*
	These variables are only to be used by the hash_check() thread.
	*/
	std::string thread_file_hash;
	std::string thread_file_path;

	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	std::string file_hash;        //hash tree root hash
	std::string file_name;        //name of the file
	std::string file_path;        //path to write file to
	boost::uint64_t file_size;           //size of the file(bytes)
	boost::uint64_t last_block;          //the last block number
	unsigned int last_block_size; //holds the exact size of the last fileBlock(in bytes)

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
		If a server sends a corrupt block it is put in the blacklist and this is
		set to true to indicate that any more communication with the server before
		it is disconnected should be ignored.
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
		std::deque<boost::uint64_t> requested_blocks;
	};

	//socket number mapped to connection special pointer
	std::map<int, connection_special> Connection_Special;

	/*
	hash_check  - checks partial download integrity (run in thread spawned in ctor)
	write_block - writes a file block
	*/
	void hash_check();
	void write_block(boost::uint64_t block_number, std::string & block);

	request_generator Request_Generator;
	hash_tree Hash_Tree;
};
#endif
