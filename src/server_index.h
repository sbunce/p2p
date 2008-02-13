/*
This class contains a thread which takes care of indexing the shared directories.
Functions which are accessed by this thread have _thread appended to them and
should not be mixed, or called by, non _thread functions.
*/

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
	file_info   - sets file_size and file_path based on file_ID
	            - returns true if file found, false if file not found
	start       - starts the index share thread
	stop        - stops the index share thread (blocks until thread stopped, up to 1 second)
	*/
	bool is_indexing();
	bool file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path);
	void start();
	void stop();

private:
	atomic<bool> stop_thread; //if true this will trigger thread termination
	atomic<int> threads;      //how many threads are currently running
	atomic<bool> indexing;    //true if server_index is currently indexing files

	//wrappers to call back functions
	static int file_info_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->file_info_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	//call backs
	void file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name);

	//used by call backs to communicate back to functions
	unsigned long file_info_file_size;
	std::string file_info_file_path;
	bool file_info_entry_exists;

	//pointer for all DB accesses not done by the indexing thread
	sqlite3 * sqlite3_DB;

	//generates hash trees
	hash_tree Hash_Tree;

/************************* BEGIN threaded *************************************/
	/*
	add_entry_thread           - adds an entry to the database if none exists for the file
	index_share_thread         - removes files listed in index that don't exist in share
	index_share_recurse_thread - recursive function to locate all files in share(calls add_entry)
	remove_missing_thread      - used by index_share_thread, removes files from the database that aren't present in the share
	*/
	void add_entry_thread(const int & size, const std::string & path);
	void index_share_thread();
	int index_share_recurse_thread(const std::string directory_name);
	void remove_missing_thread();

	//pointer for all DB access by the indexing thread
	sqlite3 * sqlite3_DB_thread;

	//wrappers to call back functions
	static int add_entry_thread_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->add_entry_thread_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	static int remove_missing_thread_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->remove_missing_thread_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	//call backs
	void add_entry_thread_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	void remove_missing_thread_call_back(int & columns_retrieved, char ** query_response, char ** column_name);

	//used by call backs to communicate back to functions
	bool add_entry_thread_entry_exists;
/************************* END threaded *************************************/
};
#endif

