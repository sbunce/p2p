//THREADSAFE
#ifndef H_DATABASE_TABLE_SHARE
#define H_DATABASE_TABLE_SHARE

//custom
#include "database.hpp"
#include "settings.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

//standard
#include <deque>
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
		downloading, //file incomplete
		complete     //file complete (all blocks hash checked good)
	};

	class info
	{
	public:
		info(){}
		info(
			const std::string & hash_in,
			const std::string & path_in,
			const boost::uint64_t file_size_in,
			const std::time_t last_write_time_in,
			const state file_state_in
		):
			hash(hash_in),
			path(path_in),
			file_size(file_size_in),
			last_write_time(last_write_time_in),
			file_state(file_state_in)
		{}
		std::string hash;
		std::string path;
		boost::uint64_t file_size;
		std::time_t last_write_time;
		state file_state;
	};

	/*
	add:
		Add info to share table.
	lookup:
		Returns info that corresponds to hash. Returns empty shared_ptr if there
		is no info.
	remove:
		Removes record for file with specified path.
	resume:
		Returns all info in the share table.
	*/
	static void add(const info & FI,
		database::pool::proxy DB = database::pool::proxy());
	static boost::shared_ptr<info> lookup(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());
	static void remove(const std::string & path,
		database::pool::proxy DB = database::pool::proxy());
	static std::deque<info> resume(database::pool::proxy DB = database::pool::proxy());

private:
	share(){}
};
}//end of namespace table
}//end of namespace database
#endif
