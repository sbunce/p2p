#ifndef H_CLIENT_SERVER_BRIDGE
#define H_CLIENT_SERVER_BRIDGE

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "DB_download.h"
#include "global.h"

//std
#include <map>
#include <set>
#include <string>

class client_server_bridge
{
public:
	enum download_mode{
		DOWNLOAD_HASH_TREE,
		DOWNLOAD_FILE,
		NOT_DOWNLOADING
	};

	/*
	Every time the client receives a block for a file it will call this function
	to update what blocks are available for downloading.
	*/
	static void download_block_received(const std::string & hash, const boost::uint64_t & block_number)
	{
		init();
		Client_Server_Bridge->download_block_receieved_priv(hash, block_number);
	}

	/*
	Client uses this to indicate it has finished downloading a file. This is
	called in the download_file dtor.
	*/
	static void finish_download(const std::string & hash)
	{
		init();
		Client_Server_Bridge->finish_download_priv(hash);
	}

	/*
	The server uses this function to determine if a file block from a downloading
	file is available to send. Some reasons for a block not being available:
	1. The block is past EOF.
	2. The block is all NULLs (filler to enable writing a leading block)
	*/
	static bool is_available(const std::string & hash, const boost::uint64_t & block_number)
	{
		init();
		return Client_Server_Bridge->is_available_priv(hash, block_number);
	}

	/*
	Returns true if a file or hash tree corresponding to hash is currently downloading.
	*/
	static download_mode is_downloading(const std::string & hash)
	{
		init();
		return Client_Server_Bridge->is_downloading_priv(hash);
	}

	/*
	If hash downloading returns true and sets path to path of hash tree file.
	*/
	static bool is_downloading_hash(const std::string & hash, std::string & path)
	{
		init();
		return Client_Server_Bridge->is_downloading_hash_priv(hash, path);
	}

	/*
	If file downloading returns true and sets path to path of file.
	PERFORMANCE: A database call is in this function.
	*/
	static bool is_downloading_file(const std::string & hash, std::string & path, boost::uint64_t & size)
	{
		init();
		return Client_Server_Bridge->is_downloading_file_priv(hash, path, size);
	}

	/*
	Client calls this to indicate a download_hash_tree has started. All file
	downloads always start by downloading the hash_tree. This should be called in
	the ctor of the download_hash_tree class.
	*/
	static void start_download(const std::string & hash)
	{
		init();
		Client_Server_Bridge->start_download_priv(hash);
	}

	/*
	When the download transitions from downloading the hash_tree to downloading
	the file this should be called.
	*/
	static void transition_download(const std::string & hash)
	{
		init();
		Client_Server_Bridge->transition_download_priv(hash);
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

	class download_state
	{
	public:
		download_state(
			const download_mode & DM_in
		):
			DM(DM_in),
			highest_contiguous(0)
		{}

		download_mode DM;    //DOWNLOAD_HASH_TREE or DOWNLOAD_FILE
		bool first_received; //true when block 0 has been received

		/*
		Example: If blocks 0, 1, 2, 3, 7, 12 received highest_contiguous would be
		equal to 3 and received blocks would contain 7, 12.

		Highest contiguous is always the last block of the contiguous part of the
		file which contains no unreceived blocks.

		The recieved_blocks set keeps track of the non-contiguous front of the
		file that blocks are getting added to.
		*/
		boost::uint64_t highest_contiguous;
		std::set<boost::uint64_t> received_blocks;

		void add_block(const boost::uint64_t block_number)
		{
			if(block_number == 0){
				first_received = true;
			}else{
				received_blocks.insert(block_number);
			}

			//remove contiguous blocks from received_blocks
			while(true){
				if(received_blocks.empty()){
					break;
				}
				if(highest_contiguous + 1 == *(received_blocks.begin())){
					highest_contiguous = highest_contiguous + 1;
					received_blocks.erase(received_blocks.begin());
				}else{
					break;
				}
			}
		}

		bool is_available(const boost::uint64_t block_number)
		{
			if(block_number == 0 && first_received){
				//block zero is available
				return true;
			}else if(block_number == 0 && !first_received){
				//block zero not yet available
				return false;
			}else if(block_number <= highest_contiguous){
				//block is within the contiguous chunk of the file
				return true;
			}

			//block is not in the contiguous chunk, check the non-configous leading edge
			std::set<boost::uint64_t>::iterator iter = received_blocks.find(block_number);
			if(iter != received_blocks.end()){
				return true;
			}else{
				return false;
			}
		}

		void transition_download()
		{
			DM = DOWNLOAD_FILE;
			first_received = false;
			highest_contiguous = 0;
			received_blocks.clear();
		}
	};

	//hash associated with download state
	std::map<std::string, download_state> Download;

	/*
	These functions all correspond to public member functions.
	*/
	void download_block_receieved_priv(const std::string & hash, const boost::uint64_t & block_number);
	void finish_download_priv(const std::string & hash);
	bool is_available_priv(const std::string & hash, const boost::uint64_t & block_number);
	download_mode is_downloading_priv(const std::string & hash);
	bool is_downloading_hash_priv(const std::string & hash, std::string & path);
	bool is_downloading_file_priv(const std::string & hash, std::string & path, boost::uint64_t & size);
	void start_download_priv(const std::string & hash);
	void transition_download_priv(const std::string & hash);

	DB_download DB_Download;
};
#endif
