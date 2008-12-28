#include "DB_download.h"

DB_download::DB_download()
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, global::DB_TIMEOUT) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	//make download table
	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size TEXT, server TEXT)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

bool DB_download::lookup_hash(const std::string & hash, boost::uint64_t & file_size)
{
	boost::mutex::scoped_lock lock(Mutex);
	lookup_hash_entry_exists = false;
	lookup_hash_file_size = &file_size;

	//locate the record
	std::ostringstream query;
	query << "SELECT size FROM download WHERE hash = '" << hash << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), lookup_hash_0_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	return lookup_hash_entry_exists;
}

bool DB_download::lookup_hash(const std::string & hash, std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);
	lookup_hash_entry_exists = false;
	lookup_hash_path = &path;

	//locate the record
	std::ostringstream query;
	query << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), lookup_hash_1_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	return lookup_hash_entry_exists;
}

bool DB_download::lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size)
{
	boost::mutex::scoped_lock lock(Mutex);
	lookup_hash_entry_exists = false;
	lookup_hash_path = &path;
	lookup_hash_file_size = &file_size;

	//locate the record
	std::ostringstream query;
	query << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), lookup_hash_2_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	return lookup_hash_entry_exists;
}

void DB_download::lookup_hash_0_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	lookup_hash_entry_exists = true;
	std::istringstream size_iss(query_response[0]);
	size_iss >> *lookup_hash_file_size;
}

void DB_download::lookup_hash_1_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	lookup_hash_entry_exists = true;
	*lookup_hash_path = global::DOWNLOAD_DIRECTORY + query_response[0];
}

void DB_download::lookup_hash_2_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	lookup_hash_entry_exists = true;
	*lookup_hash_path = global::DOWNLOAD_DIRECTORY + query_response[0];

	std::istringstream size_iss(query_response[1]);
	size_iss >> *lookup_hash_file_size;
}

void DB_download::resume(std::vector<download_info> & resume_DL)
{
	boost::mutex::scoped_lock lock(Mutex);
	resume_resume_ptr = &resume_DL;
	if(sqlite3_exec(sqlite3_DB, "SELECT * FROM download", resume_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_download::resume_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	namespace fs = boost::filesystem;

	std::string path(query_response[1]);
	path = global::DOWNLOAD_DIRECTORY + path;

	//make sure the file is present
	std::fstream fstream_temp(path.c_str());
	if(fstream_temp.is_open()){
		fstream_temp.close();
		fs::path path_boost = fs::system_complete(fs::path(global::DOWNLOAD_DIRECTORY + query_response[1], fs::native));

		std::istringstream size_iss(query_response[2]);
		boost::uint64_t size;
		size_iss >> size;
		resume_resume_ptr->push_back(
			download_info(
				query_response[0], //hash
				query_response[1], //name
				size               //size
			)
		);

		//get servers
		char delims[] = ";";
		char * result = NULL;
		result = strtok(query_response[3], delims);
		while(result != NULL){
			resume_resume_ptr->back().IP.push_back(result);
			result = strtok(NULL, delims);
		}
	}else{
		//partial file removed, delete entry from database
		std::ostringstream query;
		query << "DELETE FROM download WHERE name = '" << query_response[1] << "'";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
			logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		}
	}
}

bool DB_download::is_downloading(const std::string & hash)
{
	bool downloading = false;
	std::ostringstream query;
	query << "SELECT 1 FROM download WHERE hash = '" << hash << "' LIMIT 1";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), is_downloading_call_back, &downloading, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	return downloading;
}

bool DB_download::start_download(const download_info & info)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(is_downloading(info.hash)){
		return false;
	}else{
		std::ostringstream query;
		query << "INSERT INTO download (hash, name, size, server) VALUES ('" << info.hash << "', '" << info.name << "', " << info.size << ", '";
		for(int x = 0; x < info.IP.size(); ++x){
			if(x+1 == info.IP.size()){
				query << info.IP[x];
			}else{
				query << info.IP[x] << ";";
			}
		}
		query << "')";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
			logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
		}
		return true;
	}
}

void DB_download::terminate_download(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::ostringstream query;
	query << "DELETE FROM download WHERE hash = '" << hash << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}
