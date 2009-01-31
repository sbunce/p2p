#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/utility.hpp>

//custom
#include "atomic_bool.hpp"
#include "atomic_int.hpp"
#include "client_server_bridge.hpp"
#include "database.hpp"
#include "global.hpp"
#include "hash_tree.hpp"
#include "sha.hpp"
#include "thread_pool.hpp"

//std
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

class server_index : private boost::noncopyable
{
public:
	server_index();
	~server_index();

	/*
	is_indexing - true if indexing(scanning for new files and hashing)
	*/
	bool is_indexing();

private:
	/*
	WARNING: All private functions and data should only be used by indexing_thread.
	The only exception to this is variable: indexing
	*/
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
	Returned by is_indexing. This indicates the server_index is hashing files.
	*/
	atomic_bool indexing;

	/*
	When fist instantiated this is set to false so that no hash trees are
	generated as a result of caling check_path() on a path. This lets us feed
	check path all the paths in the database to build the initial tree.
	*/
	bool generate_hash_tree_disabled;

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

	/*
	Used upon main_loop start to populate share tree.
	*/
	int path_call_back(int columns_retrieved, char ** response, char ** column_name);

	database::connection DB;
	database::table::share DB_Share;
	database::table::preferences DB_Preferences;
	hash_tree Hash_Tree;
};
#endif

