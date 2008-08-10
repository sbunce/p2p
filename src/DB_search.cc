#include "DB_search.h"

DB_search::DB_search()
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS search (hash TEXT, name TEXT, size TEXT, server TEXT)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS search_hash_index ON search (hash)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS search_name_index ON search (name)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_search::search(std::string & search_word, std::vector<download_info> & search_results)
{
	boost::mutex::scoped_lock lock(Mutex);
	search_results.clear();
	search_results_ptr = &search_results;

	//convert the asterisk to a % for more common wildcard notation
	for(std::string::iterator iter0 = search_word.begin(); iter0 != search_word.end(); ++iter0){
		if(*iter0 == '*'){
			*iter0 = '%';
		}
	}
	logger::debug(LOGGER_P1,"search entered: '",search_word,"'");
	if(!search_word.empty()){
		std::ostringstream query;
		query << "SELECT * FROM search WHERE name LIKE '%" << search_word << "%'";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), search_call_back_wrapper, (void *)this, NULL) != 0){
			logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		}
	}
}

void DB_search::search_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	std::istringstream size_iss(query_response[2]);
	boost::uint64_t size;
	size_iss >> size;
	download_info Download_Info(
		query_response[0], //hash
		query_response[1], //name
		size,              //size
		0,                 //latest request
		0
	);

	//get servers
	char delims[] = ";";
	char * result = NULL;
	result = strtok(query_response[3], delims);
	while(result != NULL){
		Download_Info.IP.push_back(result);
		result = strtok(NULL, delims);
	}
	search_results_ptr->push_back(Download_Info);
}

void DB_search::get_servers(const std::string & hash, std::vector<std::string> & servers)
{
	boost::mutex::scoped_lock lock(Mutex);
	get_servers_results_ptr = &servers;
	std::ostringstream query;
	query << "SELECT server FROM search WHERE hash = '" << hash << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), get_servers_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_search::get_servers_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	char delims[] = ";";
	char * result = NULL;
	result = strtok(query_response[0], delims);
	while(result != NULL){
		get_servers_results_ptr->push_back(result);
		result = strtok(NULL, delims);
	}
}
