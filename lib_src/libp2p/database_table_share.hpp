//THREADSAFE
#ifndef H_DATABASE_TABLE_SHARE
#define H_DATABASE_TABLE_SHARE

//custom
#include "database.hpp"
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
	enum state{
		downloading, //0 - downloading, incomplete tree
		complete     //1 - complete, hash tree complete and checked
	};

	class file_info
	{
	public:
		file_info();
		file_info(
			const std::string & hash_in,
			const boost::uint64_t & file_size_in,
			const std::string & path_in,
			const state State_in
		);

		std::string hash;
		boost::uint64_t file_size;
		std::string path;
		state State;
	};

	/*
	add_entry:
		Add entry to the DB.
	delete_entry:
		Removes entry from database. Removes hash tree if there are no more
		references to it.
	lookup_hash:
		Lookup a record in the share table by it's hash. An empty shared_ptr is
		returned if no record exists.
	lookup_path:
		Lookup a record in the share table by it's path. An empty shared_ptr is
		returned if no record exists.
	resume:
		Used on program start to retrieve file_info for all files in share where
		state = DOWNLOADING.
	*/
	static void add_entry(const file_info & FI,
		database::pool::proxy DB = database::pool::proxy());
	static void delete_entry(const std::string & path,
		database::pool::proxy DB = database::pool::proxy());
	static boost::shared_ptr<file_info> lookup_hash(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());
	static boost::shared_ptr<file_info> lookup_path(const std::string & path,
		database::pool::proxy DB = database::pool::proxy());
	static void resume(std::vector<file_info> & Resume,
		database::pool::proxy DB = database::pool::proxy());

private:
	share(){}
};
}//end of namespace table
}//end of namespace database
#endif
