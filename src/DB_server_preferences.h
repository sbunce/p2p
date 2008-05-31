#ifndef H_DB_SERVER_PREFERENCES
#define H_DB_SERVER_PREFERENCES

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

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
	unsigned int get_speed_limit_uint();
	void set_speed_limit(const unsigned int & speed_limit);
	int get_max_connections();
	void set_max_connections(const unsigned int & max_connections);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	void get_share_directory_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_share_directory_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_server_preferences * this_class = (DB_server_preferences *)obj_ptr;
		this_class->get_share_directory_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::string get_share_directory_download_directory;

	void get_speed_limit_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_speed_limit_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_server_preferences * this_class = (DB_server_preferences *)obj_ptr;
		this_class->get_speed_limit_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::string get_speed_limit_speed_limit;

	void get_max_connections_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_max_connections_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_server_preferences * this_class = (DB_server_preferences *)obj_ptr;
		this_class->get_max_connections_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::string get_max_connections_max_connections;
};
#endif
