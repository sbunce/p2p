//NOT-THREADSAFE, THREAD SPAWNING
#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

//custom
#include "convert.hpp"
#include "block_arbiter.hpp"
#include "database.hpp"
#include "download.hpp"
#include "download_info.hpp"
#include "global.hpp"
#include "hash_tree.hpp"
#include "request_generator.hpp"

//include
#include <atomic_bool.hpp>
#include <atomic_int.hpp>
#include <singleton.hpp>

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
	virtual void pause();
	virtual unsigned percent_complete();
	virtual void register_connection(const download_connection & DC);
	virtual void remove();
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used);
	virtual bool response(const int & socket, std::string message);
	virtual const boost::uint64_t size();
	virtual void unregister_connection(const int & socket);

private:

	//used by the hashing thread
	boost::thread hashing_thread;                 //thread to check file
	atomic_bool hashing;                          //true if hashing
	atomic_int<int> hashing_percent;              //percent done hashing
	atomic_int<boost::uint64_t> first_unreceived; //block at end of file upon download start + 1

	//used to let only one file hash at a time
	class hashing_mutex: public singleton<hashing_mutex>
	{
		friend class singleton<hashing_mutex>;
	public:
		~hashing_mutex(){}
		boost::mutex & Mutex(){ return *_Mutex; }
	private:
		hashing_mutex(): _Mutex(new boost::mutex()){}
		boost::mutex * _Mutex;
	};

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
		char slot_ID;        //slot ID the server gave for the file

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
	hash_check:
		Checks partial download integrity (run in thread spawned in ctor).
	protocol_block:
		Handles an incoming hash block (message with P_BLOCK command).
	protocl_wait:
		Handles an incoming wait command that was sent in response to a block
		request.
	protocol_slot:
		Handles receiving response to slot request.
	write_block:
		Writes a file block.
	*/
	void hash_check(hash_tree::tree_info & Tree_Info, std::string file_path);
	void protocol_block(std::string & message, connection_special * conn);
	void protocol_wait(std::string & message, connection_special * conn);
	void protocol_slot(std::string & message, connection_special * conn);
	void write_block(boost::uint64_t block_number, std::string & block);

	boost::shared_ptr<request_generator> Request_Generator;
	database::connection DB;
	download_info Download_Info;
	hash_tree Hash_Tree;
};
#endif
