#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

//custom
#include "client_server_bridge.h"
#include "DB_share.h"
#include "DB_server_preferences.h"
#include "global.h"
#include "locking_smart_pointer.h"
#include "hash_tree.h"
#include "sha.h"

//std
#include <ctime>
#include <deque>
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
	is_indexing - true if indexing(scanning for new files and hashing)
	*/
	bool is_indexing();

private:
	locking_smart_pointer<bool> stop_thread;  //if true this will trigger thread termination
	locking_smart_pointer<int> threads;       //how many threads are currently running
	locking_smart_pointer<bool> indexing;     //true if server_index is currently indexing files

	/*
	generate_hash - generates hash tree for file
	index_share   - removes files listed in index that don't exist in share
	scan_hashes   - scans hash directory
	scan_share    - recursively scan a share directory
	*/
	void generate_hash(const boost::filesystem::path & file_path);
	void index_share();
	void scan_hashes();
	void scan_share(std::string directory_name);

	DB_share DB_Share;
	DB_server_preferences DB_Server_Preferences;
	hash_tree Hash_Tree;
};
#endif

