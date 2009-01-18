#ifndef H_CLIENT_SERVER_BRIDGE
#define H_CLIENT_SERVER_BRIDGE

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "atomic_bool.h"
#include "contiguous_set.h"
#include "global.h"

//std
#include <map>
#include <set>
#include <string>

class client_server_bridge
{
public:
	/*
	Client calls this to indicate a download_hash_tree has started. All file
	downloads always start by downloading the hash_tree. This is called in the
	ctor of the download_hash_tree class.

	block_num: highest hash_block available for upload
	*/
	static void start_hash_tree(const std::string & hash)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		Client_Server_Bridge->start_hash_tree_priv(hash);
	}

	/*
	This is called when a file starts downloading.
	*/
	static void start_file(const std::string & hash, const boost::uint64_t & file_block_count)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		Client_Server_Bridge->start_file_priv(hash, file_block_count);
	}

	/*
	Client calls this to indicate that either a download was cancelled, or that
	a download finished normally. This function gets rid of all information
	pertaining to the download_hash_tree or download_file contained in the
	client_server_buffer.
	*/
	static void finish_download(const std::string & hash)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		Client_Server_Bridge->finish_download_priv(hash);
	}

	/*
	The hash tree keeps track of what blocks are available to send. Available hash
	blocks are always contiguous starting at beginning of file. The
	download_hash_tree calls this function to update the highest hash block
	available.
	*/
	static void update_hash_tree_highest(const std::string & hash, const boost::uint64_t & block_num)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		Client_Server_Bridge->update_hash_tree_highest_priv(hash, block_num);
	}

	/*
	Every time the client receives a block for a file it will call this function
	to update what blocks are available for downloading.
	*/
	static void add_file_block(const std::string & hash, const boost::uint64_t & block_num)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		Client_Server_Bridge->add_file_block_priv(hash, block_num);
	}

	//returns true if hash tree is downloading
	static bool is_downloading(const std::string & hash)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		return Client_Server_Bridge->is_downloading_priv(hash);
	}

	enum download_state{
		NOT_DOWNLOADING,           //file is not downloading, but it may be complete and in share
		DOWNLOADING_NOT_AVAILABLE, //file is downloading but block not available
		DOWNLOADING_AVAILABLE      //file is downloading and block is available
	};

	/*
	The server uses this function to determine if a hash block from a downloading
	hash tree is available to send.
	*/
	static download_state hash_block_available(const std::string & hash, const boost::uint64_t & block_num)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		return Client_Server_Bridge->hash_block_available_priv(hash, block_num);
	}

	/*
	The server uses this function to determine if a file block from a downloading
	file is available to send.
	*/
	static download_state file_block_available(const std::string & hash, const boost::uint64_t & block_num)
	{
		boost::mutex::scoped_lock lock(Mutex);
		init();
		return Client_Server_Bridge->file_block_available_priv(hash, block_num);
	}

	/*
	The client must resume all downloads before server indexing starts. Otherwise
	if the server index hits a downloading file not yet resumed it will remove the
	hash tree for it.
	*/
	static void unblock_server_index()
	{
		server_index_blocked = false;
	}
	static bool server_index_is_blocked()
	{
		return server_index_blocked;
	}

private:
	client_server_bridge();

	//init() must be called at the top of every public function
	static void init()
	{
		if(Client_Server_Bridge == NULL){
			Client_Server_Bridge = new client_server_bridge();
		}
	}

	//the one possible instance of DB_blacklist
	static client_server_bridge * Client_Server_Bridge;
	static boost::mutex Mutex; //mutex for all static public functions

	//used by server_index_is_blocked() to indicate whether indexing can start
	static atomic_bool server_index_blocked;

	//used to keep track of progress of downloading hash tree
	class hash_tree_state
	{
	public:
		hash_tree_state():initialized(false){}
		bool initialized;                  //when false no blocks available
		boost::uint64_t highest_available; //any block <= this is available
	};
	//std::map<root hash, info>
	std::map<std::string, hash_tree_state> Hash_Tree_State;

	//used to keep track of progress of downloading file
	class file_state
	{
	public:
		//end should be set to file block count
		file_state(const boost::uint64_t & end):
			initialized(false),
			Contiguous(0, end)
		{}
		//when false no blocks available
		bool initialized;

		//keeps track of leading edge of blocks
		contiguous_set<boost::uint64_t> Contiguous;
	};
	//std::map<root hash, info>
	std::map<std::string, file_state> File_State;

	/*
	These functions all correspond to public member functions.
	*/
	void start_hash_tree_priv(const std::string & hash);
	void start_file_priv(const std::string & hash, const boost::uint64_t & file_block_count);
	void finish_download_priv(const std::string & hash);
	void update_hash_tree_highest_priv(const std::string & hash, const boost::uint64_t & block_num);
	void add_file_block_priv(const std::string & hash, const boost::uint64_t & block_num);
	bool is_downloading_priv(const std::string & hash);
	download_state file_block_available_priv(const std::string & hash, const boost::uint64_t & block_num);
	download_state hash_block_available_priv(const std::string & hash, const boost::uint64_t & block_number);
};
#endif
