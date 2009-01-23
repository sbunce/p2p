#ifndef H_DB_SHARE
#define H_DB_SHARE

//boost
#include <boost/tuple/tuple.hpp>

//custom
#include "atomic_bool.h"
#include "database.h"
#include "DB_hash.h"
#include "global.h"
#include "hash_tree.h"

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
	add_entry      - adds an entry to the database
	delete_hash    - removes entry from database
	*/
	void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void delete_entry(const std::string & path);

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
	database DB;

	/*
	unique_key - returns true key is unique in share table
	*/
	bool unique_hash(const std::string & hash);

	DB_hash DB_Hash;
};
#endif
