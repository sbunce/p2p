#ifndef H_DB_SHARE
#define H_DB_SHARE

//boost
#include <boost/tuple/tuple.hpp>

//custom
#include "atomic_bool.h"
#include "DB_hash.h"
#include "global.h"
#include "hash_tree.h"
#include "sqlite3_wrapper.h"

//std
#include <fstream>
#include <set>
#include <sstream>
#include <string>

class DB_share
{
public:
	DB_share();

	/*
	Functions to modify the database:
	add_entry      - adds an entry to the database if none exists for the file
	                 NOTE: key must be for a complete hash tree
	delete_hash    - 
	remove_missing - removes files from the database that aren't present in the share
	*/
	void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void delete_entry(const std::string & hash, const std::string & path);
	void remove_missing(const std::string & share_directory);

	/*
	Lookup functions, the first parameter is what to look up the record by.
	lookup_<*>  - look up information by <*>, returns false if no info exists
	*/
	bool lookup_hash(const std::string & hash);
	bool lookup_hash(const std::string & hash, std::string & path);
	bool lookup_hash(const std::string & hash, boost::uint64_t & size);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & size);
	bool lookup_path(const std::string & path, std::string & hash, boost::uint64_t & size);

private:
	sqlite3_wrapper::database DB;

	/*
	remove_missing_call_back - call back for remove missing
	unique_key - returns true key is unique in share table
	*/
	int remove_missing_call_back(std::map<std::string, std::string> & missing,
		int columns_retrieved, char ** response, char ** column_name);
	bool unique_hash(const std::string & hash);

	DB_hash DB_Hash;
};
#endif
