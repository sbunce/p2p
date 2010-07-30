#ifndef H_DB_TABLE_JOIN
#define H_DB_TABLE_JOIN

//custom
#include "db_all.hpp"

//standard
#include <set>
#include <sstream>
#include <string>

namespace db{
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
		db::pool::proxy DB = db::pool::singleton()->get());
	static std::set<std::string> resume_hash(const std::string & peer_ID,
		db::pool::proxy DB = db::pool::singleton()->get());

private:
	join(){}
};
}//end of namespace table
}//end of namespace database
#endif
