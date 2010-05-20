#include "db_table_source.hpp"

void db::table::source::add(const std::string & remote_ID,
	const std::string hash, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO source VALUES('" << remote_ID << "', '" << hash << "')";
	DB->query(ss.str());
}
