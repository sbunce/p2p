#include "DB_search.h"

DB_search::DB_search()
{
	DB.query("CREATE TABLE IF NOT EXISTS search (hash TEXT, name TEXT, size TEXT, server TEXT)");
	DB.query("CREATE INDEX IF NOT EXISTS search_hash_index ON search (hash)");
	DB.query("CREATE INDEX IF NOT EXISTS search_name_index ON search (name)");
}

static int search_call_back(std::vector<download_info> & search_results, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2] && response[3]);
	std::istringstream size_iss(response[2]);
	boost::uint64_t size;
	size_iss >> size;
	download_info Download_Info(
		response[0], //hash
		response[1], //name
		size,        //size
		0,           //latest request
		0
	);

	//get servers
	char delims[] = ";";
	char * result = NULL;
	result = strtok(response[3], delims);
	while(result != NULL){
		Download_Info.IP.push_back(result);
		result = strtok(NULL, delims);
	}
	search_results.push_back(Download_Info);
	return 0;
}

void DB_search::search(std::string & search_term, std::vector<download_info> & search_results)
{
	search_results.clear();

	//use more common wildcard notation
	std::replace(search_term.begin(), search_term.end(), '*', '%');

	LOGGER << "search entered: '" << search_term << "'";
	if(!search_term.empty()){
		std::ostringstream query;
		query << "SELECT hash, name, size, server FROM search WHERE name LIKE '%"
			<< search_term << "%'";
		DB.query(query.str(), &search_call_back, search_results);
	}
}

static int get_servers_call_back(std::vector<std::string> & servers, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	char delims[] = ";";
	char * result = NULL;
	result = strtok(response[0], delims);
	while(result != NULL){
		servers.push_back(result);
		result = strtok(NULL, delims);
	}
	return 0;
}

void DB_search::get_servers(const std::string & hash, std::vector<std::string> & servers)
{
	std::ostringstream query;
	query << "SELECT server FROM search WHERE hash = '" << hash << "'";
	DB.query(query.str(), &get_servers_call_back, servers);
}
