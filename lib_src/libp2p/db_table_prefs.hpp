#ifndef H_DB_TABLE_PREFS
#define H_DB_TABLE_PREFS

//custom
#include "db_all.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/thread.hpp>

//standard
#include <string>

namespace db{
namespace table{
class prefs
{
public:
	/*
	Functions are named after what key they deal with.
	*/

	//get a value
	static unsigned get_max_download_rate(
		db::pool::proxy DB = db::pool::proxy());
	static unsigned get_max_connections(
		db::pool::proxy DB = db::pool::proxy());
	static unsigned get_max_upload_rate(
		db::pool::proxy DB = db::pool::proxy());
	static std::string get_ID(
		db::pool::proxy DB = db::pool::proxy());
	static std::string get_port(
		db::pool::proxy DB = db::pool::proxy());

	//set a value
	static void set_max_download_rate(const unsigned rate,
		db::pool::proxy DB = db::pool::proxy());
	static void set_max_connections(const unsigned connections,
		db::pool::proxy DB = db::pool::proxy());
	static void set_max_upload_rate(const unsigned rate,
		db::pool::proxy DB = db::pool::proxy());
	static void set_port(const std::string & port,
		db::pool::proxy DB = db::pool::proxy());

private:
	prefs(){}
};
}//end of namespace table
}//end of namespace database
#endif
