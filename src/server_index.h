#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tokenizer.hpp>

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

	class directory_contents
	{
	public:
		directory_contents(){}
		std::map<std::string, directory_contents> Directory; //directories within directory
		std::map<std::string, boost::uint64_t> File;         //files within directory (name mapped to size)
	};

	//root node of directory tree
	std::map<std::string, directory_contents> Root;

	/*
	The ctor starts a thread in this function which initiates all indexing
	functions.
	*/
	void main_loop();

	/*
	Attempts to add a path to the directory tree. If the path doesn't exist in
	the tree the file will be hashed and added to the tree.
	*/
	void check_path(const std::string & path);
	void check_path_recurse(const bool & is_file, const boost::uint64_t & file_size,
		std::map<std::string, directory_contents> & Directory, std::deque<std::string> & tokens,
		const std::string & path);

	/*
	Scans the whole directory tree. If a file is missing it will be removed from
	the tree and the entry for the file will be removed from the database. Also,
	if the file changed size the file will be removed from the database and the
	file will be rehashed.

	Note: check_missing() initiates tree traversal on root node.
	*/
	void check_missing();
	void check_missing_recurse_1(std::map<std::string, directory_contents> & Directory, std::string current_directory);
	bool check_missing_recurse_2(directory_contents & DC, std::string current_directory);

	/*
	generate_hash_tree - generates a hash tree and adds information to database
	scan_share         - calls check_path() on every file in share
	tokenize_path      - breaks up a path in to tokens, ex: /full/path -> "full", "path" (with no quotes/comma)
	*/
	void generate_hash_tree(const std::string & file_path);
	void scan_share(std::string directory_name);
	void tokenize_path(const std::string & path, std::deque<std::string> & tokens);

	DB_share DB_Share;
	DB_preferences DB_Preferences;
	hash_tree Hash_Tree;
};
#endif

