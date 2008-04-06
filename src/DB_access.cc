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
	int return_code;

	//open DB
	if((return_code = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to open DB, sqlite error: ", return_code);
	}

	//DB timeout to 1 second
	if((return_code = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to set timeout, sqlite error: ",return_code);
	}

	//make share table if it doesn't already exist
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size INTEGER, path TEXT)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create table 'share', sqlite error: ",return_code);
	}
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS path_index ON share (path)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create index 'path_index', sqlite error: ",return_code);
	}

	//make download table if it doesn't already exist
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create table 'download', sqlite error: ",return_code);
	}
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS hash_index ON download (hash)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create index 'hash_index', sqlite error: ",return_code);
	}

	//make search table if it doesn't already exist
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS search (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create table 'search', sqlite error: ",return_code);
	}
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS search_hash_index ON search (hash)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create index 'search_hash_index', sqlite_error: ",return_code);
	}
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS search_name_index ON search (name)", NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"failed to create index 'search_name_index', sqlite error: ",return_code);
	}
}

void DB_access::share_add_entry(const int & size, const std::string & path)
{
	std::ostringstream query;
	share_add_entry_entry_exists = false;

	//determine if the entry already exists
	query << "SELECT * FROM share WHERE path LIKE \"" << path << "\" LIMIT 1";

	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), share_add_entry_call_back_wrapper, (void *)this, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on select, sqlite error: ",return_code);
	}

	if(!share_add_entry_entry_exists){
		query.str("");
		query << "INSERT INTO share (hash, size, path) VALUES ('" << Hash_Tree.create_hash_tree(path) << "', '" << size << "', '" << path << "')";

		if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
			global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on insert, sqlite error: ",return_code);
		}
	}
}

void DB_access::share_add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	share_add_entry_entry_exists = true;
}

bool DB_access::share_file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path)
{
	std::ostringstream query;
	share_file_info_entry_exists = false;

	//locate the record
	query << "SELECT size, path FROM share WHERE ID LIKE \"" << file_ID << "\" LIMIT 1";

	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), share_file_info_call_back_wrapper, (void *)this, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on select, sqlite error: ", return_code);
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
	share_file_info_file_size = strtoul(query_response[0], NULL, 0);
	share_file_info_file_path.assign(query_response[1]);
}

void DB_access::share_remove_missing()
{
	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, "SELECT path FROM share", share_remove_missing_call_back_wrapper, (void *)this, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on select, sqlite error: ",return_code);
	}
}

void DB_access::share_remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	std::fstream temp(query_response[0]);

	if(temp.is_open()){
		temp.close();
	}
	else{
		std::ostringstream query;
		query << "DELETE FROM share WHERE path = \"" << query_response[0] << "\"";

		int return_code;
		if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
			global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on delete, sqlite error: ",return_code);
		}
	}
}

bool DB_access::download_get_file_path(const std::string & hash, std::string & path)
{
	std::ostringstream query;
	download_get_file_path_entry_exits = false;

	//locate the record
	query << "SELECT name FROM download WHERE hash = \"" << hash << "\" LIMIT 1";
	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), download_get_file_path_call_back_wrapper, (void *)this, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on select, sqlite error: ",return_code);
	}

	if(download_get_file_path_entry_exits){
		path = global::CLIENT_DOWNLOAD_DIRECTORY + download_get_file_path_file_name;
		return true;
	}
	else{
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"download record not found");
	}
}

void DB_access::download_get_file_path_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	download_get_file_path_entry_exits = true;
	download_get_file_path_file_name.assign(query_response[0]);
}

void DB_access::download_initial_fill_buff(std::list<download_info_buffer> & resumed_download)
{
	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, "SELECT * FROM download", download_initial_fill_buff_call_back_wrapper, (void *)this, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on select, sqlite error: ",return_code);
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

		download_info_buffer IB;
		IB.resumed = true;

		IB.hash.assign(query_response[0]);
		IB.file_name.assign(query_response[1]);
		fs::path path_boost = fs::system_complete(fs::path(global::CLIENT_DOWNLOAD_DIRECTORY + IB.file_name, fs::native));
		IB.file_size.assign(query_response[2]);
		unsigned int currentBytes = fs::file_size(path_boost);
		IB.latest_request = currentBytes / (global::P_BLS_SIZE - 1); //(global::P_BLS_SIZE - 1) because control size is 1 byte

		//get servers
		char delims[] = ",";
		char * result = NULL;
		result = strtok(query_response[3], delims);
		while(result != NULL){
			IB.server_IP.push_back(result);
			result = strtok(NULL, delims);
		}

		//get file_ID's associated with servers
		result = strtok(query_response[4], delims);
		while(result != NULL){
			IB.file_ID.push_back(result);
			result = strtok(NULL, delims);
		}

		download_initial_fill_buff_resumed_download.push_back(IB);
	}
	else{ //partial file removed, delete entry from database

		std::ostringstream query;
		query << "DELETE FROM download WHERE name = \"" << query_response[1] << "\"";

		int return_code;
		if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
			global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on delete, sqlite error: ",return_code);
		}
	}
}

bool DB_access::download_start_download(download_info_buffer & info)
{
	std::ostringstream query;
	query << "INSERT INTO download (hash, name, size, server_IP, file_ID) VALUES ('" << info.hash << "', '" << info.file_name << "', " << info.file_size << ", '";
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
	query << "')";

	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) == 0){
		return true;
	}
	else{
		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"query error on insert, sqlite error: ",return_code);
		return false;
	}
}

void DB_access::download_terminate_download(const std::string & hash)
{
	std::ostringstream query;
	query << "DELETE FROM download WHERE hash = \"" << hash << "\"";

	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on delete, sqlite error: ",return_code);
	}
}

void DB_access::search(std::string & search_word, std::list<download_info_buffer> & info)
{
	search_results.clear();

	//convert the asterisk to a % for more common wildcard notation
	for(std::string::iterator iter0 = search_word.begin(); iter0 != search_word.end(); ++iter0){
		if(*iter0 == '*'){
			*iter0 = '%';
		}
	}

	global::debug_message(global::INFO,__FILE__,__FUNCTION__,"search entered: \"",search_word,"\"");

	std::ostringstream query;
	if(!search_word.empty()){
		query << "SELECT * FROM search WHERE name LIKE \"%" << search_word << "%\"";

		int return_code;
		if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), search_call_back_wrapper, (void *)this, NULL)) != 0){
			global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"query error on select, sqlite error: ",return_code);
		}

		info.clear();
		info.splice(info.end(), search_results);
	}
}

void DB_access::search_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	download_info_buffer temp;
	temp.hash.append(query_response[0]);
	temp.file_name.append(query_response[1]);
	temp.file_size.append(query_response[2]);

	//get servers
	char delims[] = ",";
	char * result = NULL;
	result = strtok(query_response[3], delims);
	while(result != NULL){
		temp.server_IP.push_back(result);
		result = strtok(NULL, delims);
	}

	//get file file_ID's associated with servers
	result = strtok(query_response[4], delims);
	while(result != NULL){
		temp.file_ID.push_back(result);
		result = strtok(NULL, delims);
	}

	search_results.push_back(temp);
}
