#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <string>
#include <sqlite3.h>

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
	start       - starts index_share_thread
	*/
	bool is_indexing();
	bool file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path);
	void start();

private:
	bool indexing;

	/*
	Wrappers for call-back functions. These exist to convert the void pointer
	that sqlite3_exec allows as a call-back in to a member function pointer that
	is needed to call a member function of *this class.
	*/
	static int add_entry_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->add_entry_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	static int file_info_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->file_info_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	static int remove_missing_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		server_index * this_class = (server_index *)object_ptr;
		this_class->remove_missing_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	/*
	The call-back functions themselves. These are called by the wrappers which are
	passed to sqlite3_exec. These contain the logic of what needs to be done.
	*/
	void add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	void file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	void remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name);

	/*
	These variables are used by the call-back functions to communicate back with
	their caller functions.
	*/
	bool add_entry_entry_exists;
	bool file_info_entry_exists;
	unsigned long file_info_file_size;
	std::string file_info_file_path;

	sqlite3 * sqlite3_DB; //sqlite database pointer to be passed to functions like sqlite3_exec
	hash_tree Hash_Tree;  //generates hash trees

	/*
	add_entry           - adds an entry to the database if none exists for the file
	index_share         - removes files listed in index that don't exist in share
	                    - adds files that exist in share but no in index
	                    - checks the share every global::SERVER_REFRESH seconds
	index_share_recurse - recursive function to locate all files in share(calls add_entry)
	remove_missing      - removes files from the database that aren't present
	*/
	void add_entry(const int & size, const std::string & path);
	void index_share_thread();
	int index_share_recurse(const std::string directory_name);
	void remove_missing();
};
#endif

