//THREADSAFE
#ifndef H_DATABASE_TABLE_BLACKLIST
#define H_DATABASE_TABLE_BLACKLIST

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <atomic_int.hpp>
#include <boost/tuple/tuple.hpp>

//standard
#include <iostream>
#include <sstream>
#include <string>

namespace database{
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
		Note: For first call last_state_seen == 0.
		Postcondition: last_state_seen = blacklist_state.
	*/
	static void add(const std::string & IP,
		database::pool::proxy DB = database::pool::proxy());
	static bool is_blacklisted(const std::string & IP,
		database::pool::proxy DB = database::pool::proxy());
	static bool modified(int & last_state_seen);

private:
	blacklist(){}

	/*
	The init function is called with boost::call_once at the top of every
	function to make sure the static variables are initialized in a thread safe
	way.
	*/
	static boost::once_flag once_flag;
	static void init();

	/*
	Incremented whenever IP added to blacklist. This makes it possible to
	determine if blacklist has changed without doing a database query.
	*/
	static atomic_int<int> & blacklist_state();
};
}//end of namespace table
}//end of namespace database
#endif
