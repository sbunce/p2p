//THREADSAFE
#ifndef H_DATABASE_TABLE_BLACKLIST
#define H_DATABASE_TABLE_BLACKLIST

//boost
#include <boost/thread.hpp>

//custom
#include "database_connection.hpp"
#include "settings.hpp"

//include
#include <atomic_int.hpp>

//std
#include <iostream>
#include <sstream>
#include <string>

namespace database{
namespace table{
class blacklist
{
public:
	blacklist(){}

	/*
	add            - add IP to blacklist
	is_blacklisted - returns true if IP on blacklist
	modified       - returns true if IP added to blacklist since last check
	                 precondition: last_state_seen should be initialized to zero
	*/
	void add(const std::string & IP);
	static void add(const std::string & IP, database::connection & DB);
	bool is_blacklisted(const std::string & IP);
	static bool is_blacklisted(const std::string & IP, database::connection & DB);
	static bool modified(int & last_state_seen);

private:
	database::connection DB;

	static boost::once_flag once_flag;
	static void init()
	{
		_blacklist_state = new atomic_int<int>(0);
	}
	//do not use this directly, use the accessor function
	static atomic_int<int> * _blacklist_state;

	//accessor function
	static atomic_int<int> & blacklist_state()
	{
		boost::call_once(init, once_flag);
		return *_blacklist_state;
	}
};

}//end of table namespace
}//end of database namespace
#endif
