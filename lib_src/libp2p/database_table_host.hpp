//THREADSAFE
#ifndef H_DATABASE_TABLE_HOST
#define H_DATABASE_TABLE_HOST

//custom
#include "database.hpp"

//include
#include <boost/shared_ptr.hpp>

//standard
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace database{
namespace table{
class host
{
public:
	/*
	add:
		Add source for file with specified hash
		std::pair<IP, port>
	lookup:
		Returns all hosts associated with the hash.
		std::set<std::pair<IP, port> >
	*/
	static void add(const std::string hash, const std::pair<std::string, std::string> & host,
		database::pool::proxy DB = database::pool::proxy());
	static std::set<std::pair<std::string, std::string> > lookup(
		const std::string & hash, database::pool::proxy DB = database::pool::proxy());

private:
	host(){}
};
}//end of namespace table
}//end of namespace database
#endif
