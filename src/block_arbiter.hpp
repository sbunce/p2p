//THREADSAFE, SINGLETON
#ifndef H_BLOCK_ARBITER
#define H_BLOCK_ARBITER

//boost
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "atomic_bool.hpp"
#include "contiguous_set.hpp"
#include "global.hpp"

//std
#include <map>
#include <set>
#include <string>

class block_arbiter : private boost::noncopyable
{
public:
	enum download_state{
		NOT_DOWNLOADING,           //file is not downloading, but it may be complete and in share
		DOWNLOADING_NOT_AVAILABLE, //file is downloading but block not available
		DOWNLOADING_AVAILABLE      //file is downloading and block is available
	};

	/*
	start_hash_tree - Called when a hash tree download starts to make the
	                  downloading hash tree available for upload.
	start_file      - Called when a file download starts to make the downloading
	                  file available for upload.
	finish_download - Called when a download cancelled, or a download finished
	                  normally.
	update_hash_tree_highest - A downloading hash tree uses this to set the
	                           highest available hash block.
	add_file_block  - A downloading file uses this to add an available file block.
	is_downloading  - Returns true if a hash tree or file that corresponds to hash
	                  is downloading.
	hash_block_available    - Returns the state of a hash block.
	file_block_available    - Returns the state of a file block.
	server_index_is_blocked - Allows the server_index start indexing when returns true.
	unblock_server_index    - Tells the server_index that it may start indexing.
	                          The client must resume all downloads before calling this.
	*/
	static void start_hash_tree(const std::string & hash);
	static void start_file(const std::string & hash, const boost::uint64_t & file_block_count);
	static void finish_download(const std::string & hash);
	static void update_hash_tree_highest(const std::string & hash, const boost::uint64_t & block_num);
	static void add_file_block(const std::string & hash, const boost::uint64_t & block_num);
	static bool is_downloading(const std::string & hash);
	static download_state hash_block_available(const std::string & hash, const boost::uint64_t & block_num);
	static download_state file_block_available(const std::string & hash, const boost::uint64_t & block_num);

private:
	block_arbiter(){}

	/*
	Returns reference to mutex to lock all public static functions. A function
	is needed instead of a regular static object to avoid static initialization
	problems.
	*/
	static boost::mutex & Mutex();

	//must be called at the top of every public function to initialize singleton
	static void init();

	//the one possible instance of DB_blacklist
	static block_arbiter * Block_Arbiter;

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
