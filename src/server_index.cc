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

	int return_code;
	if((return_code = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #1 failed with sqlite3 error " << return_code << "\n";
#endif
	}
	if((return_code = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #2 failed with sqlite3 error " << return_code << "\n";
#endif
	}
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size INTEGER, path TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #3 failed with sqlite3 error " << return_code << "\n";
#endif
	}
	if((return_code = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS path_index ON share (path)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::server_index() #4 failed with sqlite3 error " << return_code << "\n";
#endif
	}

	indexing = false;
	stop_thread = false;
	threads = 0;
}

void server_index::add_entry(const int & size, const std::string & path)
{
	std::ostringstream query;
	add_entry_entry_exists = false;

	//determine if the entry already exists
	query << "SELECT * FROM share WHERE path LIKE \"" << path << "\" LIMIT 1";

	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), add_entry_call_back_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::add_entry() #1 failed with sqlite3 error " << return_code << "\n";
#endif
	}

	if(!add_entry_entry_exists){
		query.str("");
		query << "INSERT INTO share (hash, size, path) VALUES ('" << Hash_Tree.create_hash_tree(path) << "', '" << size << "', '" << path << "')";

		if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: server_index::add_entry() #2 failed with sqlite3 error " << return_code << "\n";
#endif
		}
	}
}

void server_index::add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	add_entry_entry_exists = true;
}

int server_index::index_share_recurse(const std::string directory_name)
{
	namespace fs = boost::filesystem;

	fs::path full_path = fs::system_complete(fs::path(directory_name, fs::native));

	if(!fs::exists(full_path)){
#ifdef DEBUG
		std::cout << "error: fileIndex::index_share(): can't locate " << full_path.string() << "\n";
#endif
		return -1;
	}

	if(fs::is_directory(full_path)){
		fs::directory_iterator end_iter;
		for(fs::directory_iterator directory_iter(full_path); directory_iter != end_iter; directory_iter++){
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string sub_directory;
					sub_directory = directory_name + directory_iter->leaf() + "/";
					index_share_recurse(sub_directory);
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

bool server_index::is_indexing()
{
	return indexing;
}

void server_index::index_share()
{
	++threads;

	//index when thread started
	indexing = true;
	remove_missing();
	index_share_recurse(global::SERVER_SHARE_DIRECTORY);
	indexing = false;

	int seconds_slept = 0;
	while(true){
		if(stop_thread){
			break;
		}

		sleep(1);
		++seconds_slept;

		if(seconds_slept > global::SHARE_REFRESH){
			seconds_slept = 0;
			indexing = true;
			remove_missing();
			index_share_recurse(global::SERVER_SHARE_DIRECTORY);
			indexing = false;
		}
	}

	--threads;
}

void server_index::remove_missing()
{
	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, "SELECT path FROM share", remove_missing_call_back_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::remove_missing() failed with sqlite3 error " << return_code << "\n";
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

		int return_code;
		if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: server_index::remove_missing_call_back() failed with sqlite3 error " << return_code << "\n";
#endif
		}
	}
}

void server_index::start()
{
	if(threads != 0){
		std::cout << "fatal error: server_index::start(): cannot start multiple indexing threads\n";
		exit(1);
	}

	boost::thread T(boost::bind(&server_index::index_share, this));
}

void server_index::stop()
{
	stop_thread = true;

	while(threads){
		usleep(100);
	}
}
