#ifndef H_CLIENT_SERVER_BRIDGE
#define H_CLIENT_SERVER_BRIDGE

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

//std
#include <string>

class client_server_bridge
{
public:
	/*
	The server uses this function to determine if a file block from a downloading
	file is available to send. Some reasons for a block not being available:
	1. The block is past EOF.
	2. The block is all NULLs (filler to enable writing a leading block)
	*/
	static bool file_block_available(const std::string & hash, const boost::uint64_t & block_number)
	{
		init();
		((client_server_bridge *)Client_Server_Bridge)->file_block_available_priv(hash, block_number);
	}

	/*
	Every time the client receives a block for a file it will call this function
	to update what blocks are available for downloading.
	*/
	static void file_block_receieved(const std::string & hash, const boost::uint64_t & block_number)
	{
		init();
		((client_server_bridge *)Client_Server_Bridge)->file_block_receieved_priv(hash, block_number);
	}

	/*
	Client uses this to indicate it has finished downloading a file. This is
	be called in the dtor of download_file.
	*/
	static void finish_file(const std::string & hash)
	{
		init();
		((client_server_bridge *)Client_Server_Bridge)->finish_file_priv(hash);
	}

	/*
	Client uses this to indicate it has started downloading a file.
	*/
	static void start_file(const std::string & hash)
	{
		init();
		((client_server_bridge *)Client_Server_Bridge)->start_file_priv(hash);
	}

private:
	client_server_bridge();

	//init() must be called at the top of every public function
	static void init()
	{
		boost::mutex::scoped_lock lock(init_mutex);
		if(Client_Server_Bridge == NULL){
			Client_Server_Bridge = new client_server_bridge();
		}
	}

	//the one possible instance of DB_blacklist
	static client_server_bridge * Client_Server_Bridge;
	static boost::mutex init_mutex; //mutex for init()

	//mutex for functions called by public static functions
	boost::mutex Mutex;

	/*
	These functions all correspond to public member functions.
	*/
	void start_file_priv(const std::string & hash);
	void file_block_receieved_priv(const std::string & hash, const boost::uint64_t & block_number);
	bool file_block_available_priv(const std::string & hash, const boost::uint64_t & block_number);
	void finish_file_priv(const std::string & hash);
};
#endif
