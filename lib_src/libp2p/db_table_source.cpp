#include "db_table_source.hpp"

void db::table::source::add(const std::string & remote_ID,
	const std::string hash, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO source VALUES('" << remote_ID << "', '" << hash << "')";
	DB->query(ss.str());
}

static int get_ID_call_back(int columns, char ** response, char ** column_name,
	std::list<std::string> & nodes)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "ID") == 0);
	nodes.push_back(response[0]);
	return 0;
}

std::list<std::string> db::table::source::get_ID(const std::string & hash,
	db::pool::proxy DB)
{
	std::list<std::string> nodes;
	std::stringstream ss;
	ss << "SELECT ID FROM source WHERE hash = '" << hash << "'";
	DB->query(ss.str(), boost::bind(&get_ID_call_back, _1, _2, _3, boost::ref(nodes)));
	return nodes;
}
