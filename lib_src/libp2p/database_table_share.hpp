//THREADSAFE
#ifndef H_DATABASE_TABLE_SHARE
#define H_DATABASE_TABLE_SHARE

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "settings.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/tuple/tuple.hpp>

//standard
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace database{
namespace table{
class share
{
public:
	/* Functions to modify DB
	add_entry:
		Add entry to the DB.
	clear:
		Clears the entire share table.
	delete_entry:
		Removes entry from database. Removes hash tree if there are no more
		references to it.
	*/
	static void add_entry(const std::string & hash, const boost::uint64_t & size,
		const std::string & path);
	static void clear();
	static void delete_entry(const std::string & path);

	/*
	Lookup functions, the first parameter is what to look up the record by.
	lookup_<*>  - look up information by <*>, returns false if no info exists
	*/
	static bool lookup_hash(const std::string & hash);
	static bool lookup_hash(const std::string & hash, std::string & path);
	static bool lookup_hash(const std::string & hash, boost::uint64_t & file_size);
	static bool lookup_hash(const std::string & hash, std::string & path,
		boost::uint64_t & file_size);
	static bool lookup_path(const std::string & path, std::string & hash,
		boost::uint64_t & file_size);

	/* Connection Functions
	All these functions correspond to above functions but they take a database
	connection. These are needed to do calls to multiple database functions
	within a transaction.
	*/
	static void add_entry(const std::string & hash, const boost::uint64_t & size,
		const std::string & path, database::pool::proxy & DB);
	static void clear(database::pool::proxy & DB);
	static void delete_entry(const std::string & path, database::pool::proxy & DB);
	static bool lookup_hash(const std::string & hash, database::pool::proxy & DB);
	static bool lookup_hash(const std::string & hash, std::string & path,
		database::pool::proxy & DB);
	static bool lookup_hash(const std::string & hash, boost::uint64_t & file_size,
		database::pool::proxy & DB);
	static bool lookup_hash(const std::string & hash, std::string & path,
		boost::uint64_t & file_size, database::pool::proxy & DB);
	static bool lookup_path(const std::string & path, std::string & hash,
		boost::uint64_t & file_size, database::pool::proxy & DB);

private:
	share(){}

};
}//end of namespace table
}//end of namespace database
#endif
