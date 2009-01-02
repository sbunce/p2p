#ifndef H_DB_SERVER_PREFERENCES
#define H_DB_SERVER_PREFERENCES

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"
#include "sqlite3_wrapper.h"

//std
#include <string>

class DB_server_preferences
{
public:
	DB_server_preferences();

	/*
	get_share_directory  - return the path to the directory where downloads will be saved
	set_share_directory  - sets download directory
	get_speed_limit_uint - returns the speed limit (bytes/second)
	set_speed_limit      - sets speed limit (speed_limit should be bytes/second)
	get_max_connections  - returns max connections allowed to server
	set_max_connections  - sets max connections allowed to server
	*/
	std::string get_share_directory();
	void set_share_directory(const std::string & share_directory);
	unsigned get_speed_limit();
	void set_speed_limit(const unsigned & speed_limit);
	unsigned get_max_connections();
	void set_max_connections(const unsigned & max_connections);

private:
	sqlite3_wrapper DB;

	//mutex for checking for, and creation of, the one row
	static boost::mutex Mutex;
};
#endif
