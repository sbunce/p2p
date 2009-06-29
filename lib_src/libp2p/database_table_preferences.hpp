//THREADSAFE
#ifndef H_DATABASE_TABLE_PREFERENCES
#define H_DATABASE_TABLE_PREFERENCES

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/thread.hpp>

//standard
#include <string>

namespace database{
namespace table{
class preferences
{
public:
	/*
	"get"|"set"_download_rate      - maximum download rate (bytes)
	"get"|"set"_upload_rate        - maximum upload rate (bytes)
	"get"|"set"_max_connections - how many incoming connections server allows
	*/
	static unsigned get_max_download_rate(database::connection & DB);
	static unsigned get_max_connections(database::connection & DB);
	static unsigned get_max_upload_rate(database::connection & DB);

	static void set_max_download_rate(const unsigned & rate, database::connection & DB);
	static void set_max_connections(const unsigned & connections, database::connection & DB);
	static void set_max_upload_rate(const unsigned & rate, database::connection & DB);

private:
	preferences(){}
};
}//end of namespace table
}//end of namespace database
#endif
