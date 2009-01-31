#ifndef H_DATABASE_TABLE_PREFERENCES
#define H_DATABASE_TABLE_PREFERENCES

//boost
#include <boost/thread.hpp>

//custom
#include "database_connection.hpp"
#include "global.hpp"

//std
#include <string>

namespace database{
namespace table{
class preferences
{
public:
	preferences();

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
	database::connection DB;

	//mutex for checking for, and creation of, the one row (look in ctor)
	static boost::mutex Mutex;
};
}//end of table namespace
}//end of database namespace
#endif
