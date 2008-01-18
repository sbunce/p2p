//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/tokenizer.hpp>

//std
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#include "client_index.h"

client_index::client_index()
{
	//create the download directory if it doesn't exist
	boost::filesystem::create_directory(global::CLIENT_DOWNLOAD_DIRECTORY);

	int returnCode;
	if((returnCode = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::client_index() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::client_index() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::client_index() #3 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS hashIndex ON download (hash)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::client_index() #4 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

bool client_index::get_file_path(const std::string & hash, std::string & path)
{
	std::ostringstream query;
	get_file_path_entry_exits = false;

	//locate the record
	query << "SELECT name FROM download WHERE hash = \"" << hash << "\" LIMIT 1";
	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), get_file_path_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::get_file_path() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	if(get_file_path_entry_exits){
		path = global::CLIENT_DOWNLOAD_DIRECTORY + get_file_path_file_name;
		return true;
	}
	else{
#ifdef DEBUG
		std::cout << "error: client_index::get_file_path() didn't find record\n";
#endif
		return false;
	}
}

void client_index::get_file_path_callBack(int & columns_retrieved, char ** query_response, char ** column_name)
{
	get_file_path_entry_exits = true;
	get_file_path_file_name.assign(query_response[0]);
}

void client_index::initial_fill_buff(std::list<exploration::info_buffer> & resumed_download)
{
	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, "SELECT * FROM download", initial_fill_buff_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::initial_fill_buff() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	resumed_download.splice(resumed_download.end(), initial_fill_buff_resumed_download);
}

void client_index::initial_fill_buff_callBack(int & columns_retrieved, char ** query_response, char ** column_name)
{
	namespace fs = boost::filesystem;

	std::string path(query_response[1]);
	path = global::CLIENT_DOWNLOAD_DIRECTORY + path;

	//make sure the file is present
	std::fstream fstream_temp(path.c_str());
	if(fstream_temp.is_open()){ //partial file exists, add to downloadBuffer
		fstream_temp.close();

		exploration::info_buffer IB;
		IB.resumed = true;

		IB.hash.assign(query_response[0]);
		IB.file_name.assign(query_response[1]);
		fs::path path_boost = fs::system_complete(fs::path(global::CLIENT_DOWNLOAD_DIRECTORY + IB.file_name, fs::native));
		IB.file_size.assign(query_response[2]);
		unsigned int currentBytes = fs::file_size(path_boost);
		IB.latest_request = currentBytes / (global::P_BLS_SIZE - 1); //(global::P_BLS_SIZE - 1) because control size is 1 byte

		std::string undelim_server_IP(query_response[3]);
		std::string undelim_file_ID(query_response[4]);

		boost::char_separator<char> sep(","); //delimiter for servers and file_IDs
		boost::tokenizer<boost::char_separator<char> > server_IP_tokens(undelim_server_IP, sep);
		boost::tokenizer<boost::char_separator<char> >::iterator server_IP_iter = server_IP_tokens.begin();
		while(server_IP_iter != server_IP_tokens.end()){
			IB.server_IP.push_back(*server_IP_iter++);
		}

		boost::tokenizer<boost::char_separator<char> > file_ID_tokens(undelim_file_ID, sep);
		boost::tokenizer<boost::char_separator<char> >::iterator file_ID_iter = file_ID_tokens.begin();
		while(file_ID_iter != file_ID_tokens.end()){
			IB.file_ID.push_back(*file_ID_iter++);
		}

		initial_fill_buff_resumed_download.push_back(IB);
	}
	else{ //partial file removed, delete entry from database

		std::ostringstream query;
		query << "DELETE FROM download WHERE name = \"" << query_response[1] << "\"";

		int returnCode;
		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::initial_fill_buff() failed with sqlite3 error " << returnCode << "\n";
#endif
		}
	}
}

bool client_index::start_download(exploration::info_buffer & info)
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

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) == 0){
		return true;
	}
	else{
#ifdef DEBUG
		std::cout << "error: client_index::start_download() failed with sqlite3 error " << returnCode << "\n";
#endif
		return false;
	}
}

void client_index::terminate_download(const std::string & hash)
{
	std::ostringstream query;
	query << "DELETE FROM download WHERE hash = \"" << hash << "\"";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: client_index::terminate_download() failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

