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
	Functions to modify the database:
	add_entry      - adds an entry to the database if none exists for the file
	delete_hash    - deletes the record associated with hash
	remove_missing - removes files from the database that aren't present in the share
	*/
	void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void delete_hash(const std::string & hash);
	void remove_missing(const std::string & share_directory);

	/*
	Lookup functions, the first parameter is what to look up the record by.
	hash_exists - returns true if a record exists with the specified hash, else false
	lookup_<*>  - look up information by <*>, returns false if no info exists
	*/
	bool hash_exists(const std::string & hash);
	bool lookup_path(const std::string & path, std::string & hash, boost::uint64_t & size);
	bool lookup_hash(const std::string & hash, std::string & path);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & size);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

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
	std::string * lookup_path_hash;
	boost::uint64_t * lookup_path_size;

	//sets only lookup_hash_file_path
	void lookup_hash_1_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_hash_1_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->lookup_hash_1_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	//sets both lookup_hash_file_size and lookup_hash_file_path
	void lookup_hash_2_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_hash_2_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->lookup_hash_2_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool lookup_hash_entry_exists;
	std::string * lookup_hash_path;
	boost::uint64_t * lookup_hash_size;
};
#endif
