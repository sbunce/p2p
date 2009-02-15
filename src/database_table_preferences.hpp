//THREADSAFE
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
	preferences(){}

	/*
	"get"|"set"_download_directory - directory where downloads are to be placed
	"get"|"set"_share_directory    - shared directory
	"get"|"set"_download_rate      - maximum download rate (bytes)
	"get"|"set"_upload_rate        - maximum upload rate (bytes)
	"get"|"set"_max_connections - how many incoming connections server allows
	*/
	std::string get_download_directory();
	unsigned get_max_download_rate();
	unsigned get_max_connections();
	std::string get_share_directory();
	unsigned get_max_upload_rate();

	void set_download_directory(const std::string & download_directory);
	void set_max_download_rate(const unsigned & rate);
	void set_max_connections(const unsigned & connections);
	void set_share_directory(const std::string & share_directory);
	void set_max_upload_rate(const unsigned & rate);

private:
	database::connection DB;
};
}//end of table namespace
}//end of database namespace
#endif
