#ifndef H_DB_DOWNLOAD
#define H_DB_DOWNLOAD

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>

//custom
#include "download_info.h"
#include "global.h"

//sqlite
#include "sqlite3.h"

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
	get_file_path      - sets path to file that corresponds to hash
	                     returns true if record found else false
	initial_fill_buff  - returns download information for all in progress downloads
	start_download     - adds a download to the database if it doesn't already exist
	                     returns true if download could be added, else false
	terminate_download - removes the download entry from the database
	*/
	bool get_file_path(const std::string & hash, std::string & path);
	void initial_fill_buff(std::vector<download_info> & resumed_download);
	bool start_download(const download_info & info);
	void terminate_download(const std::string & hash);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	/*
	is_downloading - returns true if hash is found in download table, else false
	*/
	bool is_downloading(const std::string & hash);

	void get_file_path_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_file_path_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_download * this_class = (DB_download *)object_ptr;
		this_class->get_file_path_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool get_file_path_entry_exits;
	std::string get_file_path_file_name;

	void initial_fill_buff_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int initial_fill_buff_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_download * this_class = (DB_download *)object_ptr;
		this_class->initial_fill_buff_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}	
	std::vector<download_info> * initial_fill_buff_resumed_download_ptr;

	static int is_downloading_call_back(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		*((bool *)object_ptr) = true;
		return 0;
	}	
};
#endif
