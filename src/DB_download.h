#ifndef H_DB_DOWNLOAD
#define H_DB_DOWNLOAD

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>

//custom
#include "download_info.h"
#include "global.h"
#include "hash_tree.h"
#include "sha.h"

//sqlite
#include <sqlite3.h>

//std
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

class DB_download
{
public:
	DB_download();

	/*
	Functions to modify the database:
	start_download     - adds a download to the database if it doesn't already exist
	                     returns true if download could be added, else false
	terminate_download - removes the download entry from the database
	*/

	bool start_download(const download_info & info);
	void terminate_download(const std::string & hash);

	/*
	Lookup functions:
	lookup_<*>  - look up information by <*>
	resume      - fill the vector with download information (used to resume downloads on program start)
	*/
	bool lookup_hash(const std::string & hash, std::string & path);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size);
	void resume(std::vector<download_info> & resume_DL);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	/*
	is_downloading - returns true if hash is found in download table, else false
	*/
	bool is_downloading(const std::string & hash);

	//callback for path only
	void lookup_hash_1_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_hash_1_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_download * this_class = (DB_download *)object_ptr;
		this_class->lookup_hash_1_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	//callback for both path and size
	void lookup_hash_2_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_hash_2_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_download * this_class = (DB_download *)object_ptr;
		this_class->lookup_hash_2_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool lookup_hash_entry_exits;
	std::string * lookup_hash_path;
	boost::uint64_t * lookup_hash_file_size;
	std::string * lookup_hash_name;

	void resume_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int resume_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_download * this_class = (DB_download *)object_ptr;
		this_class->resume_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}	
	std::vector<download_info> * resume_resume_ptr;

	static int is_downloading_call_back(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		*((bool *)object_ptr) = true;
		return 0;
	}	
};
#endif
