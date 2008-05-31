#include "DB_search.h"

DB_search::DB_search()
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,"#2 ",sqlite3_errmsg(sqlite3_DB));
	}

	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS search (hash TEXT, name TEXT, size TEXT, server_IP TEXT, file_ID TEXT);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#7 ",sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS search_hash_index ON search (hash);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#8 ",sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS search_name_index ON search (name);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#9 ",sqlite3_errmsg(sqlite3_DB));
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

	logger::debug(LOGGER_P1,"search entered: \"",search_word,"\"");

	std::ostringstream query;
	if(!search_word.empty()){
		query << "SELECT * FROM search WHERE name LIKE \"%" << search_word << "%\";";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), search_call_back_wrapper, (void *)this, NULL) != 0){
			logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		}
	}
}

void DB_search::search_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	download_info Download_Info(
		false,                                 //not resuming
		query_response[0],                     //hash
		query_response[1],                     //name
		strtoull(query_response[2], NULL, 10), //size
		0                                      //latest request
	);

	//get servers
	char delims[] = ",";
	char * result = NULL;
	result = strtok(query_response[3], delims);
	while(result != NULL){
		Download_Info.server_IP.push_back(result);
		result = strtok(NULL, delims);
	}

	//get file file_ID's associated with servers
	result = strtok(query_response[4], delims);
	while(result != NULL){
		Download_Info.file_ID.push_back(result);
		result = strtok(NULL, delims);
	}

	search_results_ptr->push_back(Download_Info);
}
