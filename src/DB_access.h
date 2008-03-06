/*
	Prefixes denote what table will be read/written by the function. For example
	a share_ function deals with the share table.
*/

#ifndef H_DB_ACCESS
#define H_DB_ACCESS

//boost
#include <boost/tokenizer.hpp>

//sqlite
#include <sqlite3.h>

//std
#include <list>
#include <string>
#include <vector>

//custom
#include "global.h"
#include "hash_tree.h"

class DB_access
{
public:
	DB_access();

	//used by download_initial_fill_buff, download_start_download, search
	class download_info_buffer
	{
	public:
		download_info_buffer()
		{
			//defaults which don't need to be set on new downloads
			resumed = false;
			latest_request = 0;
		}

		bool resumed; //if this info_buffer is being used to resume a download

		std::string hash;
		std::string file_name;
		std::string file_size;
		unsigned int latest_request; //what block was most recently requested

		//information unique to the server, these vectors are parallel
		std::vector<std::string> server_IP;
		std::vector<std::string> file_ID;
	};

	/*
	share_add_entry - adds an entry to the database if none exists for the file
	share_file_info - sets file_size and file_path based on file_ID
	                - returns true if file found else false
	remove_missing  - removes files from the database that aren't present in the share
	*/
	void share_add_entry(const int & size, const std::string & path);
	bool share_file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path);
	void share_remove_missing();

	/*
	download_get_file_path      - sets path to file that corresponds to hash
	                            - returns true if record found else false
	download_initial_fill_buff  - returns download information for all in progress downloads
	download_start_download     - adds a download to the database
	download_terminate_download - removes the download entry from the database
	*/
	bool download_get_file_path(const std::string & hash, std::string & path);
	void download_initial_fill_buff(std::list<download_info_buffer> & resumed_download);
	bool download_start_download(download_info_buffer & info);
	void download_terminate_download(const std::string & hash);

	/*
	search - searches the database for names which match search_word
	*/
	void search(std::string & search_word, std::list<download_info_buffer> & info);

private:
	//pointer for all DB accesses
	sqlite3 * sqlite3_DB;

	/*
	These functions are associated with the public functions by name prefix. For
	example share_file_info_* is associated with share_file_info public function.

	call back function
	call back wrapper
	variables associated with call back
	*/

	//BEGIN share_ stuff
	void share_file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int share_file_info_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->share_file_info_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	unsigned long share_file_info_file_size;
	std::string share_file_info_file_path;
	bool share_file_info_entry_exists;

	void share_add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int share_add_entry_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->share_add_entry_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool share_add_entry_entry_exists;

	void share_remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int share_remove_missing_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->share_remove_missing_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	//BEGIN download_ stuff
	void download_get_file_path_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int download_get_file_path_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->download_get_file_path_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool download_get_file_path_entry_exits;
	std::string download_get_file_path_file_name;

	void download_initial_fill_buff_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int download_initial_fill_buff_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->download_initial_fill_buff_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}	
	std::list<download_info_buffer> download_initial_fill_buff_resumed_download;

	//BEGIN search_ stuff
	void search_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int search_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)obj_ptr;
		this_class->search_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::list<download_info_buffer> search_results;

	//misc stuff
	hash_tree Hash_Tree; //generates hash trees
};
#endif
