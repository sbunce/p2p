#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <string>
#include <sqlite3.h>

#include "atomic.h"
#include "global.h"
#include "hash_tree.h"

class server_index
{
public:
	server_index();
	/*
	is_indexing - true if indexing(scanning for new files and hashing)
	start       - starts the index share thread
	stop        - stops the index share thread (blocks until thread stopped, up to 1 second)
	*/
	bool is_indexing();
	void start();
	void stop();

private:
	atomic<bool> stop_thread; //if true this will trigger thread termination
	atomic<int> threads;      //how many threads are currently running
	atomic<bool> indexing;    //true if server_index is currently indexing files

	//generates hash trees
	hash_tree Hash_Tree;

	//pointer for all DB access
	sqlite3 * sqlite3_DB;

	/*
	add_entry           - adds an entry to the database if none exists for the file
	index_share         - removes files listed in index that don't exist in share
	index_share_recurse - recursive function to locate all files in share(calls add_entry)
	remove_missing      - used by index_share, removes files from the database that aren't present in the share
	*/
	void add_entry(const int & size, const std::string & path);
	void index_share();
	int index_share_recurse(const std::string directory_name);
	void remove_missing();

	//call back wrappers
	static int add_entry_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->add_entry_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	static int remove_missing_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->remove_missing_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	//call backs
	void add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	void remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name);

	//used by call backs to communicate back to functions
	bool add_entry_entry_exists;
};
#endif

