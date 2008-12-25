#ifndef H_DB_CLIENT_PREFERENCES
#define H_DB_CLIENT_PREFERENCES

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

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
	unsigned int get_speed_limit();
	void set_speed_limit(const unsigned int & speed_limit);
	int get_max_connections();
	void set_max_connections(const unsigned int & max_connections);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	void get_download_directory_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_download_directory_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_client_preferences * this_class = (DB_client_preferences *)obj_ptr;
		this_class->get_download_directory_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::string get_download_directory_download_directory;

	void get_speed_limit_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_speed_limit_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_client_preferences * this_class = (DB_client_preferences *)obj_ptr;
		this_class->get_speed_limit_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::string get_speed_limit_speed_limit;

	void get_max_connections_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_max_connections_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_client_preferences * this_class = (DB_client_preferences *)obj_ptr;
		this_class->get_max_connections_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::string get_max_connections_max_connections;
};
#endif
