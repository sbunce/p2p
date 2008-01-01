//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "server_index.h"

server_index::server_index()
{
	//create the share directory if it doesn't exist
	boost::filesystem::create_directory(global::SERVER_SHARE_DIRECTORY);

	int returnCode;
	if((returnCode = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size INTEGER, path TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #3 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS pathIndex ON share (path)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #4 failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	indexing = false;
}

void server_index::add_entry(const int & size, const std::string & path)
{
	std::ostringstream query;
	add_entry_entry_exists = false;

	//determine if the entry already exists
	query << "SELECT * FROM share WHERE path LIKE \"" << path << "\" LIMIT 1";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), add_entry_call_back_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::add_entry() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	if(!add_entry_entry_exists){
		query.str("");
		query << "INSERT INTO share (hash, size, path) VALUES ('" << Hash.hash_file(path) << "', '" << size << "', '" << path << "')";

		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: server_index::add_entry() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
		}
	}
}

void server_index::add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	add_entry_entry_exists = true;
}

bool server_index::file_info(const int & file_ID, int & file_size, std::string & file_path)
{
	std::ostringstream query;
	file_info_entry_exists = false;

	//locate the record
	query << "SELECT size, path FROM share WHERE ID LIKE \"" << file_ID << "\" LIMIT 1";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), file_info_call_back_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::file_info() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	if(file_info_entry_exists){
		file_size = file_info_file_size;
		file_path = file_info_file_path;
		return true;
	}
	else{
		return false;
	}
}

void server_index::file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	file_info_entry_exists = true;
	file_info_file_size = atoi(query_response[0]);
	file_info_file_path.assign(query_response[1]);
}

bool server_index::is_indexing()
{
	return indexing;
}

void server_index::index_share_thread()
{
	while(true){
		indexing = true;
		remove_missing();
		index_share_recurse(global::SERVER_SHARE_DIRECTORY);
		indexing = false;
		sleep(global::SHARE_REFRESH);
	}
}

int server_index::index_share_recurse(const std::string directory_name)
{
	namespace fs = boost::filesystem;

	fs::path fullPath = fs::system_complete(fs::path(directory_name, fs::native));

	if(!fs::exists(fullPath)){
#ifdef DEBUG
		std::cout << "error: fileIndex::index_share(): can't locate " << fullPath.string() << "\n";
#endif
		return -1;
	}

	if(fs::is_directory(fullPath)){

		fs::directory_iterator end_iter;

		for(fs::directory_iterator directory_iter(fullPath); directory_iter != end_iter; directory_iter++){
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string subDirectory;
					subDirectory = directory_name + directory_iter->leaf() + "/";
					index_share_recurse(subDirectory);
				}
				else{
					//determine size
					fs::path file_path = fs::system_complete(fs::path(directory_name + directory_iter->leaf(), fs::native));
					add_entry(fs::file_size(file_path), file_path.string());
				}
			}
			catch(std::exception & ex){
#ifdef DEBUG
				std::cout << "error: server_index::index_share_recurse(): file " << directory_iter->leaf() << " caused exception " << ex.what() << "\n";
#endif
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: fileIndex::index_share(): index points to file when it should point at directory\n";    
#endif
	}

	return 0;
}

void server_index::remove_missing()
{
	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, "SELECT path FROM share", remove_missing_call_back_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::remove_missing() failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

void server_index::remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	std::fstream temp(query_response[0]);

	if(temp.is_open()){
		temp.close();
	}
	else{
		std::ostringstream query;
		query << "DELETE FROM share WHERE path = \"" << query_response[0] << "\"";

		int returnCode;
		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: server_index::remove_missing_call_back() failed with sqlite3 error " << returnCode << "\n";
#endif
		}
	}
}

void server_index::start()
{
	boost::thread T(boost::bind(&server_index::index_share_thread, this));
}

