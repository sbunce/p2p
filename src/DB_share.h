#ifndef H_DB_SHARE
#define H_DB_SHARE

//boost
#include <boost/tuple/tuple.hpp>

//C
#include <cstdio>

//custom
#include "global.h"
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
	delete_hash    - deletes all records with hash
	remove_missing - removes files from the database that aren't present in the share
	*/
	void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void delete_hash(const std::string & hash, const std::string & path);
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
	sqlite3_wrapper DB;

	/*
	remove_missing_call_back - call back for remove missing
	*/
	int remove_missing_call_back(std::set<std::string> & missing_hashes,
		int columns_retrieved, char ** response, char ** column_name);
};
#endif
