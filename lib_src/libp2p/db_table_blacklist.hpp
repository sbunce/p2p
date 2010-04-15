#ifndef H_DB_TABLE_BLACKLIST
#define H_DB_TABLE_BLACKLIST

//custom
#include "db_all.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <atomic_int.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>

//standard
#include <iostream>
#include <sstream>
#include <string>

namespace db{
namespace table{
class blacklist
{
public:
	/*
	add:
		Add IP to blacklist.
	is_blacklisted:
		Returns true if IP is blacklisted.
	modified:
		Returns true if last_state_seen doesn't match the current blacklist_state.
		Note: To not check the blacklist the first call initialize last_state_seen
		to zero, or not zero to check on the first call.
		Postcondition: last_state_seen = blacklist_state.
	*/
	static void add(const std::string & IP,
		db::pool::proxy DB = db::pool::proxy());
	static bool is_blacklisted(const std::string & IP,
		db::pool::proxy DB = db::pool::proxy());
	static bool modified(int & last_state_seen);

private:
	blacklist(){}

	class static_wrap
	{
	public:
		static_wrap();

		/*
		blacklist_state:
			Incremented whenever IP added to blacklist. This makes it possible to
			determine if blacklist has changed without doing a database query.
		*/
		atomic_int<int> & blacklist_state();
	private:
		static boost::once_flag once_flag;

		//initialize all static objects, called by ctor
		static void init();

		static atomic_int<int> & _blacklist_state();
	};
};
}//end of namespace table
}//end of namespace database
#endif
