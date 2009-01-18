#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

//custom
#include "atomic_bool.h"
#include "atomic_int.h"
#include "client_server_bridge.h"
#include "DB_preferences.h"
#include "DB_share.h"
#include "global.h"
#include "hash_tree.h"
#include "sha.h"
#include "thread_pool.h"

//std
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>
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
	boost::thread indexing_thread;
	atomic_bool indexing; //true if server_index is currently indexing files

	/*
	generate_hash - generates hash tree for file
	index_share   - removes files listed in index that don't exist in share
	scan_share    - recursively scan a share directory
	*/
	void generate_hash_tree(const std::string & file_path);
	void index_share();
	void scan_share(std::string directory_name);

	DB_share DB_Share;
	DB_preferences DB_Preferences;
	hash_tree Hash_Tree;
};
#endif

