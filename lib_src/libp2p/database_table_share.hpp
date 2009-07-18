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
	lookup_<lookup by this>:
		The first parameter is the key by which to lookup the data. The non-const
		references will be set if the record was found. True is returned if the
		record is found (and the referenced variables are set), or false is
		returned if no record is found (and referenced variables are no set).
	*/
	static void add_entry(const std::string & hash, const boost::uint64_t & size,
		const std::string & path, database::pool::proxy DB = database::pool::proxy());
	static void clear(database::pool::proxy DB = database::pool::proxy());
	static void delete_entry(const std::string & path,
		database::pool::proxy DB = database::pool::proxy());
	static bool lookup_hash(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());
	static bool lookup_hash(const std::string & hash, std::string & path,
		database::pool::proxy DB = database::pool::proxy());
	static bool lookup_hash(const std::string & hash, boost::uint64_t & file_size,
		database::pool::proxy DB = database::pool::proxy());
	static bool lookup_hash(const std::string & hash, std::string & path,
		boost::uint64_t & file_size, database::pool::proxy DB = database::pool::proxy());
	static bool lookup_path(const std::string & path, std::string & hash,
		boost::uint64_t & file_size, database::pool::proxy DB = database::pool::proxy());

private:
	share(){}

	/*
	The once_func is only called once. It sets up the table.
	*/
	static boost::once_flag once_flag;
	static void once_func(database::pool::proxy & DB);
};
}//end of namespace table
}//end of namespace database
#endif
