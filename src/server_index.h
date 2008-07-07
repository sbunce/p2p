#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//custom
#include "DB_share.h"
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
	is_indexing - true if indexing(scanning for new files and hashing)
	*/
	bool is_indexing();

private:
	volatile bool stop_thread; //if true this will trigger thread termination
	volatile int threads;      //how many threads are currently running
	bool indexing;             //true if server_index is currently indexing files

	/*
	index_share         - removes files listed in index that don't exist in share
	index_share_recurse - recursive function to locate all files in share(calls add_entry)
	*/
	void generate_hash(const boost::filesystem::path & file_path);
	void index_share();
	void index_share_recurse(std::string directory_name);

	DB_share DB_Share;
	hash_tree Hash_Tree;
};
#endif

