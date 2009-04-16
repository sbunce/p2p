//THREADSAFE
#ifndef H_DATABASE_TABLE_BLACKLIST
#define H_DATABASE_TABLE_BLACKLIST

//boost
#include <boost/thread.hpp>

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <atomic_int.hpp>
#include <database_connection.hpp>

//std
#include <iostream>
#include <sstream>
#include <string>

namespace database{
namespace table{
class blacklist : private boost::noncopyable
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
		Postcondition: last_state_seen == blacklist_state.
	*/
	static void add(const std::string & IP, database::connection & DB);
	static bool is_blacklisted(const std::string & IP, database::connection & DB);
	static bool modified(int & last_state_seen);

private:
	blacklist(){}

	/*
	Static variables and means to initialize them. Do not use these variables
	direclty. Use the accessor functions.
	*/
	static boost::once_flag once_flag;
	static void init();
	static atomic_int<int> * _blacklist_state;


	/* Accessor functions for static variables.
	blacklist_state:
		Incremented whenever IP added to blacklist. This makes it possible to
		determine if blacklist has changed without doing a database query.
	*/
	static atomic_int<int> & blacklist_state();
};
}//end of table namespace
}//end of database namespace
#endif
