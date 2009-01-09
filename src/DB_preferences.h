#ifndef H_DB_PREFERENCES
#define H_DB_PREFERENCES

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"
#include "sqlite3_wrapper.h"

//std
#include <string>

class DB_preferences
{
public:
	DB_preferences();

	/*
	"get"|"set"_download_directory - directory where downloads are to be placed
	"get"|"set"_share_directory    - shared directory
	"get"|"set"_download_rate      - maximum download rate (bytes)
	"get"|"set"_upload_rate        - maximum upload rate (bytes)
	"get"|"set"_client_connection  - how many connections client is allowed to make
	"get"|"set"_server_connections - how many incoming connections server allows
	*/
	unsigned get_client_connections();
	std::string get_download_directory();
	unsigned get_download_rate();
	unsigned get_server_connections();
	std::string get_share_directory();
	unsigned get_upload_rate();

	void set_client_connections(const unsigned & connections);
	void set_download_directory(const std::string & download_directory);
	void set_download_rate(const unsigned & rate);
	void set_server_connections(const unsigned & connections);
	void set_share_directory(const std::string & share_directory);
	void set_upload_rate(const unsigned & rate);

private:
	sqlite3_wrapper DB;

	//mutex for checking for, and creation of, the one row
	static boost::mutex Mutex;
};
#endif
