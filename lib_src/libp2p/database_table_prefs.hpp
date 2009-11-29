//THREADSAFE
#ifndef H_DATABASE_TABLE_PREFS
#define H_DATABASE_TABLE_PREFS

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
class prefs
{
public:
	/*
	"get"|"set"_download_rate:
		Maximum download rate (bytes).
	"get"|"set"_upload_rate:
		Maximum upload rate (bytes).
	"get"|"set"_max_connections:
		How many connections allowed.
	*/
	static unsigned get_max_download_rate(
		database::pool::proxy DB = database::pool::proxy());
	static unsigned get_max_connections(
		database::pool::proxy DB = database::pool::proxy());
	static unsigned get_max_upload_rate(
		database::pool::proxy DB = database::pool::proxy());
	static void set_max_download_rate(const unsigned rate,
		database::pool::proxy DB = database::pool::proxy());
	static void set_max_connections(const unsigned connections,
		database::pool::proxy DB = database::pool::proxy());
	static void set_max_upload_rate(const unsigned rate,
		database::pool::proxy DB = database::pool::proxy());

private:
	prefs(){}
};
}//end of namespace table
}//end of namespace database
#endif
