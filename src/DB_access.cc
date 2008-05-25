//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "DB_access.h"

DB_access::DB_access()
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,"#2 ",sqlite3_errmsg(sqlite3_DB));
	}

	//make share table if it doesn't already exist
	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size TEXT, path TEXT);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#3 ",sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS path_index ON share (path);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#4 ",sqlite3_errmsg(sqlite3_DB));
	}

	//make download table if it doesn't already exist
	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size TEXT, server_IP TEXT, file_ID TEXT);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#5 ",sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS hash_index ON download (hash);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#6 ",sqlite3_errmsg(sqlite3_DB));
	}

	//make search table if it doesn't already exist
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

void DB_access::share_add_entry(const std::string & hash, const uint64_t & size, const std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);

	//if this is set to true after the query the entry exists in the database
	share_add_entry_entry_exists = false;

	//determine if the entry already exists
	char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
	std::ostringstream query;
	query << "SELECT * FROM share WHERE path = \"" << path_sqlite << "\" LIMIT 1;";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), share_add_entry_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}
	sqlite3_free(path_sqlite);

	if(!share_add_entry_entry_exists){
		char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
		std::ostringstream query;
		query << "INSERT INTO share (hash, size, path) VALUES ('" << hash << "', '" << size << "', '" << path_sqlite << "');";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
			logger::debug(LOGGER_P1,"#2 ",sqlite3_errmsg(sqlite3_DB));
		}
		sqlite3_free(path_sqlite);
	}
}

void DB_access::share_add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	share_add_entry_entry_exists = true;
}

void DB_access::share_delete_hash(const std::string & hash)
{
	std::ostringstream query;
	query << "DELETE FROM share WHERE hash = \"" << hash << "\";";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}
}

bool DB_access::share_file_exists(const std::string & path, std::string & existing_hash, uint64_t & existing_size)
{
	share_file_exists_cond = false;
	share_file_exists_hash_ptr = &existing_hash;
	share_file_exists_size_ptr = &existing_size;

	char * query = sqlite3_mprintf("SELECT hash, size FROM share WHERE path = \"%q\" LIMIT 1;", path.c_str());
	if(sqlite3_exec(sqlite3_DB, query, share_file_exists_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	sqlite3_free(query);
	return share_file_exists_cond;
}

void DB_access::share_file_exists_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	share_file_exists_cond = true;
	*share_file_exists_hash_ptr = query_response[0];
	*share_file_exists_size_ptr = strtoull(query_response[1], NULL, 10);
}

bool DB_access::share_file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path)
{
	boost::mutex::scoped_lock lock(Mutex);

	share_file_info_entry_exists = false;

	//locate the record
	std::ostringstream query;
	query << "SELECT size, path FROM share WHERE ID LIKE \"" << file_ID << "\" LIMIT 1;";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), share_file_info_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	if(share_file_info_entry_exists){
		file_size = share_file_info_file_size;
		file_path = share_file_info_file_path;
		return true;
	}
	else{
		return false;
	}
}

void DB_access::share_file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	share_file_info_entry_exists = true;
	share_file_info_file_size = strtoull(query_response[0], NULL, 10);
	share_file_info_file_path.assign(query_response[1]);
}

void DB_access::share_remove_missing()
{
	if(sqlite3_exec(sqlite3_DB, "SELECT hash, path FROM share;", share_remove_missing_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_access::share_remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	boost::mutex::scoped_lock lock(Mutex);

	std::fstream fin(query_response[1]);
	if(!fin.is_open()){
		//remove hash tree
		namespace fs = boost::filesystem;
		fs::path path = fs::system_complete(fs::path(global::HASH_DIRECTORY+std::string(query_response[0]), fs::native));
		fs::remove(path);

		std::ostringstream query;
		query << "DELETE FROM share WHERE hash = \"" << query_response[0] << "\";";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
			logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		}
	}
}

bool DB_access::download_get_file_path(const std::string & hash, std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);

	download_get_file_path_entry_exits = false;

	//locate the record
	std::ostringstream query;
	query << "SELECT name FROM download WHERE hash = \"" << hash << "\" LIMIT 1;";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), download_get_file_path_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	if(download_get_file_path_entry_exits){
		path = global::CLIENT_DOWNLOAD_DIRECTORY + download_get_file_path_file_name;
		return true;
	}
	else{
		logger::debug(LOGGER_P1,"download record not found");
	}
}

void DB_access::download_get_file_path_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	download_get_file_path_entry_exits = true;
	download_get_file_path_file_name.assign(query_response[0]);
}

void DB_access::download_initial_fill_buff(std::list<download_info> & resumed_download)
{
	boost::mutex::scoped_lock lock(Mutex);

	if(sqlite3_exec(sqlite3_DB, "SELECT * FROM download;", download_initial_fill_buff_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	resumed_download.splice(resumed_download.end(), download_initial_fill_buff_resumed_download);
}

void DB_access::download_initial_fill_buff_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	namespace fs = boost::filesystem;

	std::string path(query_response[1]);
	path = global::CLIENT_DOWNLOAD_DIRECTORY + path;

	//make sure the file is present
	std::fstream fstream_temp(path.c_str());
	if(fstream_temp.is_open()){ //partial file exists, add to downloadBuffer
		fstream_temp.close();

		fs::path path_boost = fs::system_complete(fs::path(global::CLIENT_DOWNLOAD_DIRECTORY + query_response[1], fs::native));
		uint64_t current_bytes = fs::file_size(path_boost);
		uint64_t latest_request = current_bytes / (global::P_BLOCK_SIZE - 1); //(global::P_BLOCK_SIZE - 1) because control size is 1 byte

		download_info Download_Info(
			true,                                  //resuming
			query_response[0],                     //hash
			query_response[1],                     //name
			strtoull(query_response[2], NULL, 10), //size
			latest_request                         //latest_request
		);

		//get servers
		char delims[] = ",";
		char * result = NULL;
		result = strtok(query_response[3], delims);
		while(result != NULL){
			Download_Info.server_IP.push_back(result);
			result = strtok(NULL, delims);
		}

		//get file_ID's associated with servers
		result = strtok(query_response[4], delims);
		while(result != NULL){
			Download_Info.file_ID.push_back(result);
			result = strtok(NULL, delims);
		}

		download_initial_fill_buff_resumed_download.push_back(Download_Info);
	}
	else{ //partial file removed, delete entry from database
		std::ostringstream query;
		query << "DELETE FROM download WHERE name = \"" << query_response[1] << "\";";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
			logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		}
	}
}

bool DB_access::download_start_download(download_info & info)
{
	boost::mutex::scoped_lock lock(Mutex);

	std::ostringstream query;
	query << "INSERT INTO download (hash, name, size, server_IP, file_ID) VALUES ('" << info.hash << "', '" << info.name << "', " << info.size << ", '";
	for(std::vector<std::string>::iterator iter0 = info.server_IP.begin(); iter0 != info.server_IP.end(); ++iter0){
		if(iter0 + 1 != info.server_IP.end()){
			query << *iter0 << ",";
		}
		else{
			query << *iter0;
		}
	}
	query << "', '";
	for(std::vector<std::string>::iterator iter0 = info.file_ID.begin(); iter0 != info.file_ID.end(); ++iter0){
		if(iter0 + 1 != info.file_ID.end()){
			query << *iter0 << ",";
		}
		else{
			query << *iter0;
		}
	}
	query << "');";

	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) == 0){
		return true;
	}
	else{
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		return false;
	}
}

void DB_access::download_terminate_download(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);

	std::ostringstream query;
	query << "DELETE FROM download WHERE hash = \"" << hash << "\";";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_access::search(std::string & search_word, std::vector<download_info> & search_results)
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

void DB_access::search_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
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
