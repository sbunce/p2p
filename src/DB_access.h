/*
Prefixes denote what table will be read/written by the function. For example
a share_ function deals with the share table.

All access to public functions must be Mutex'd.
*/

#ifndef H_DB_ACCESS
#define H_DB_ACCESS

//boost
#include <boost/thread/thread.hpp>

//sqlite
#include <sqlite3.h>

//std
#include <list>
#include <string>
#include <vector>

//custom
#include "download_info.h"
#include "global.h"

class DB_access
{
public:
	DB_access();

	/*
	share_add_entry   - adds an entry to the database if none exists for the file
	share_delete_hash - deletes the record associated with hash
	share_file_info   - sets file_size and file_path based on file_ID
	                  - returns true if file found else false
	share_file_exists - returns true and sets existing_hash and existing_size if the file exists in the database
	remove_missing    - removes files from the database that aren't present in the share
	*/
	void share_add_entry(const std::string & hash, const uint64_t & size, const std::string & path);
	void share_delete_hash(const std::string & hash);
	bool share_file_exists(const std::string & path, std::string & existing_hash, uint64_t & existing_size);
	bool share_file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path);
	void share_remove_missing();

	/*
	download_get_file_path      - sets path to file that corresponds to hash
	                            - returns true if record found else false
	download_initial_fill_buff  - returns download information for all in progress downloads
	download_start_download     - adds a download to the database
	                            - returns false if the download already started
	download_terminate_download - removes the download entry from the database
	*/
	bool download_get_file_path(const std::string & hash, std::string & path);
	void download_initial_fill_buff(std::list<download_info> & resumed_download);
	bool download_start_download(download_info & info);
	void download_terminate_download(const std::string & hash);

	/*
	search - searches the database for names which match search_word
	*/
	void search(std::string & search_word, std::vector<download_info> & info);

private:
	sqlite3 * sqlite3_DB; //pointer for all DB accesses
	boost::mutex Mutex;   //mutex for all public functions

	/*
	These functions are associated with the public functions by name prefix. For
	example share_file_info_* is associated with share_file_info public function.

	call back function
	call back wrapper
	variables associated with call back
	*/

	//BEGIN share_ stuff
	void share_file_exists_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int share_file_exists_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->share_file_exists_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool share_file_exists_cond;
	std::string * share_file_exists_hash_ptr;
	uint64_t * share_file_exists_size_ptr;

	void share_file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int share_file_info_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->share_file_info_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	uint64_t share_file_info_file_size;
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
	std::list<download_info> download_initial_fill_buff_resumed_download;

	//BEGIN search_ stuff
	void search_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int search_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)obj_ptr;
		this_class->search_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::vector<download_info> * search_results_ptr;
};
#endif
