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
	Functions are named after what key they deal with.
	*/

	//get a value
	static unsigned get_max_download_rate(
		database::pool::proxy DB = database::pool::proxy());
	static unsigned get_max_connections(
		database::pool::proxy DB = database::pool::proxy());
	static unsigned get_max_upload_rate(
		database::pool::proxy DB = database::pool::proxy());
	static std::string get_ID(
		database::pool::proxy DB = database::pool::proxy());
	static std::string get_port(
		database::pool::proxy DB = database::pool::proxy());

	//set a value
	static void set_max_download_rate(const unsigned rate,
		database::pool::proxy DB = database::pool::proxy());
	static void set_max_connections(const unsigned connections,
		database::pool::proxy DB = database::pool::proxy());
	static void set_max_upload_rate(const unsigned rate,
		database::pool::proxy DB = database::pool::proxy());
	static void set_port(const std::string & port,
		database::pool::proxy DB = database::pool::proxy());

private:
	prefs(){}
};
}//end of namespace table
}//end of namespace database
#endif
