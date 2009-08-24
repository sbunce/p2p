//THREADSAFE
#ifndef H_DATABASE_TABLE_SHARE
#define H_DATABASE_TABLE_SHARE

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "settings.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

//standard
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace database{
namespace table{
class share
{
public:
	class file_info
	{
	public:
		std::string hash;
		//std::
	};

	/*
	add_entry:
		Add entry to the DB.
	delete_entry:
		Removes entry from database. Removes hash tree if there are no more
		references to it.
	lookup_<lookup by this>:
		The first parameter is the key by which to lookup the data. The non-const
		references will be set if the record was found. True is returned if the
		record is found (and the referenced variables are set), or false is
		returned if no record is found (and referenced variables are no set).
	*/
	static void add_entry(const std::string & hash, const boost::uint64_t & file_size,
		const std::string & path, database::pool::proxy DB = database::pool::proxy());
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
/*
	static void resume(std::vector<boost::shared_ptr<slot_element> > & Resume,
		database::pool::proxy DB = database::pool::proxy());
*/
private:
	share(){}
};
}//end of namespace table
}//end of namespace database
#endif
