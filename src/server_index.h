#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

//custom
#include "DB_share.h"
#include "DB_server_preferences.h"
#include "global.h"
#include "hash_tree.h"

//std
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class server_index
{
public:
	server_index();
	~server_index();

	/*
	is_indexing         - true if indexing(scanning for new files and hashing)
	set_share_directory - sets directory to be shared
	*/
	bool is_indexing();
	void set_share_directory(const std::string & directory);

private:
	volatile bool stop_thread;  //if true this will trigger thread termination
	volatile int threads;       //how many threads are currently running
	bool indexing;              //true if server_index is currently indexing files
	volatile bool change_share; //stops current indexing so that share_directory can be changed

	//shared directory to be indexed
	std::string share_directory;
	boost::mutex SD_mutex; //mutex for access to share_directory in index_share()

	/*
	generate_hash       - generates hash tree for file
	index_share         - removes files listed in index that don't exist in share
	index_share_recurse - recursive function to locate all files in share(calls add_entry)
	*/
	void generate_hash(const boost::filesystem::path & file_path);
	void index_share();
	void index_share_recurse(std::string directory_name);

	DB_share DB_Share;
	DB_server_preferences DB_Server_Preferences;
	hash_tree Hash_Tree;
};
#endif

