#include "database_table_host.hpp"

void database::table::host::add(const std::string hash,
	const std::pair<std::string, std::string> & host, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO host(hash, IP, port) VALUES('" << hash << "', '"
		<< host.first << "', '" << host.second << "')";
	DB->query(ss.str());
}

static int lookup_call_back(std::set<std::pair<std::string, std::string> > * host,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(columns_retrieved == 2);
	assert(std::strcmp(column_name[0], "IP") == 0);
	assert(std::strcmp(column_name[1], "port") == 0);
	host->insert(std::make_pair(response[0], response[1]));
	return 0;
}

std::set<std::pair<std::string, std::string> > database::table::host::lookup(
	const std::string & hash, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT IP, port FROM host WHERE hash = '" << hash << "'";
	std::set<std::pair<std::string, std::string> > host;
	DB->query(ss.str(), &lookup_call_back, &host);
	return host;
}
