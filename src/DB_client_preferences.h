#ifndef H_DB_CLIENT_PREFERENCES
#define H_DB_CLIENT_PREFERENCES

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"
#include "sqlite3_wrapper.h"

//std
#include <string>

class DB_client_preferences
{
public:
	DB_client_preferences();

	/*
	get_download_directory - return the path to the directory where downloads will be saved
	set_download_directory - sets download directory
	get_speed_limit_uint   - returns the speed limit (bytes/second)
	set_speed_limit        - sets speed limit (speed_limit should be bytes/second)
	get_max_connections    - returns max connections client allowed to make
	set_max_connections    - sets max connections client allowed to make
	*/
	std::string get_download_directory();
	void set_download_directory(const std::string & download_directory);
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
