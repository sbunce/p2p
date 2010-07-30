#ifndef H_DB_TABLE_SOURCE
#define H_DB_TABLE_SOURCE

//custom
#include "db_all.hpp"

//include
#include <boost/shared_ptr.hpp>

//standard
#include <set>
#include <sstream>
#include <string>

namespace db{
namespace table{
class source
{
public:
	/*
	add:
		Add ID/hash pair.
	get_ID:
		Get ID of node that has file.
	*/
	static void add(const std::string & remote_ID, const std::string hash,
		db::pool::proxy DB = db::pool::singleton()->get());
	static std::list<std::string> get_ID(const std::string & hash,
		db::pool::proxy DB = db::pool::singleton()->get());

private:
	source(){}
};
}//end of namespace table
}//end of namespace database
#endif
