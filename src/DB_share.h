#ifndef H_DB_SHARE
#define H_DB_SHARE

//boost
#include <boost/thread/mutex.hpp>

//C
#include <cstdio>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

//std
#include <fstream>
#include <sstream>
#include <string>

class DB_share
{
public:
	DB_share();

	/*
	add_entry      - adds an entry to the database if none exists for the file
	delete_hash    - deletes the record associated with hash
	hash_exists    - returns true if a record exists with the specified hash, else false
	lookup_path    - returns true and sets existing_hash and existing_size if the file corresponding to path exists in database
	lookup_hash    - returns true and sets file_size and file_path if the file corresponding to hash exists in database
	remove_missing - removes files from the database that aren't present in the share
	*/
	void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void delete_hash(const std::string & hash);
	bool hash_exists(const std::string & hash);
	bool lookup_path(const std::string & path, std::string & existing_hash, boost::uint64_t & existing_size);
	bool lookup_hash(const std::string & hash, boost::uint64_t & file_size, std::string & file_path);
	void remove_missing(const std::string & share_directory);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	void hash_exists_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int hash_exists_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->hash_exists_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool hash_exists_exists;

	void lookup_path_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_path_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->lookup_path_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool lookup_path_entry_exists;
	std::string * lookup_path_hash_ptr;
	boost::uint64_t * lookup_path_size_ptr;

	void lookup_hash_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_hash_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->lookup_hash_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool lookup_hash_entry_exists;
	boost::uint64_t * lookup_hash_file_size;
	std::string * lookup_hash_file_path;

	void add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int add_entry_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->add_entry_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool add_entry_entry_exists;

	void remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int remove_missing_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->remove_missing_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
};
#endif
