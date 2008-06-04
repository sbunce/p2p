#ifndef H_DB_SHARE
#define H_DB_SHARE

//boost
#include <boost/thread/thread.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

//std
#include <fstream>

class DB_share
{
public:
	DB_share();

	/*
	add_entry      - adds an entry to the database if none exists for the file
	delete_hash    - deletes the record associated with hash
	file_info      - sets file_size and file_path that corresponds to hash
	               - returns true if file found else false
	file_exists    - returns true and sets existing_hash and existing_size if the file exists in the database
	remove_missing - removes files from the database that aren't present in the share
	*/
	void add_entry(const std::string & hash, const uint64_t & size, const std::string & path);
	void delete_hash(const std::string & hash);
	bool lookup_path(const std::string & path, std::string & existing_hash, uint64_t & existing_size);
	bool lookup_hash(const std::string & hash, uint64_t & file_size, std::string & file_path);
	void remove_missing();

private:
	sqlite3 * sqlite3_DB; //pointer for all DB accesses
	boost::mutex Mutex;   //mutex for all public functions

	void lookup_path_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_path_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->lookup_path_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool lookup_path_entry_exists;
	std::string * lookup_path_hash_ptr;
	uint64_t * lookup_path_size_ptr;

	void lookup_hash_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int lookup_hash_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_share * this_class = (DB_share *)object_ptr;
		this_class->lookup_hash_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool lookup_hash_entry_exists;
	uint64_t * lookup_hash_file_size;
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
