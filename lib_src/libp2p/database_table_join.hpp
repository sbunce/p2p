#ifndef H_DATABASE_TABLE_JOIN
#define H_DATABASE_TABLE_JOIN

//custom
#include "database.hpp"

//standard
#include <set>
#include <sstream>
#include <string>

namespace database{
namespace table{
class join
{
public:

	/*
	resume_peers:
		Returns std::pair<IP, port> of hosts that have files we need. Used on
		program start for resume.
	resume_hash:
		When a we connect to a host and it sends us it's peer_ID this function is
		called to determine what slots to open.
	*/
	static std::set<std::pair<std::string, std::string> > resume_peers(
		database::pool::proxy DB = database::pool::proxy());
	static std::set<std::string> resume_hash(const std::string & peer_ID,
		database::pool::proxy DB = database::pool::proxy());

private:
	join(){}
};
}//end of namespace table
}//end of namespace database
#endif
