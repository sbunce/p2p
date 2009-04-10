//THREADSAFE
#ifndef H_DATABASE_TABLE_SHARE
#define H_DATABASE_TABLE_SHARE

//boost
#include <boost/tuple/tuple.hpp>

//custom
#include "hash_tree.hpp"
#include "settings.hpp"

//include
#include <atomic_bool.hpp>
#include <database_connection.hpp>

//std
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace database{
namespace table{
class share
{
public:
	share();

	/*
	Functions to modify the database:
	add_entry      - adds an entry to the database
	delete_hash    - removes entry from database
	*/
	void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path);
	static void add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path, database::connection & DB);
	void delete_entry(const std::string & path);
	static void delete_entry(const std::string & path, database::connection & DB);

	/*
	Lookup functions, the first parameter is what to look up the record by.
	lookup_<*>  - look up information by <*>, returns false if no info exists
	*/
	bool lookup_hash(const std::string & hash);
	static bool lookup_hash(const std::string & hash, database::connection & DB);
	bool lookup_hash(const std::string & hash, std::string & path);
	static bool lookup_hash(const std::string & hash, std::string & path, database::connection & DB);
	bool lookup_hash(const std::string & hash, boost::uint64_t & file_size);
	static bool lookup_hash(const std::string & hash, boost::uint64_t & file_size, database::connection & DB);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size);
	static bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size, database::connection & DB);
	bool lookup_path(const std::string & path, std::string & hash, boost::uint64_t & file_size);
	static bool lookup_path(const std::string & path, std::string & hash, boost::uint64_t & file_size, database::connection & DB);

private:
	database::connection DB;

	/*
	unique_key - returns true if key is unique in share table
	*/
	static bool unique_hash(const std::string & hash, database::connection & DB);
};
}//end of table namespace
}//end of database namespace
#endif
