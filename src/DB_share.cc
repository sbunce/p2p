#include "DB_share.h"

DB_share::DB_share()
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,"#2 ",sqlite3_errmsg(sqlite3_DB));
	}

	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size TEXT, path TEXT);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#3 ",sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS path_index ON share (path);", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#4 ",sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_share::add_entry(const std::string & hash, const uint64_t & size, const std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);

	//if this is set to true after the query the entry exists in the database
	add_entry_entry_exists = false;

	//determine if the entry already exists
	char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
	std::ostringstream query;
	query << "SELECT * FROM share WHERE path = \"" << path_sqlite << "\" LIMIT 1;";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), add_entry_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}
	sqlite3_free(path_sqlite);

	if(!add_entry_entry_exists){
		char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
		std::ostringstream query;
		query << "INSERT INTO share (hash, size, path) VALUES ('" << hash << "', '" << size << "', '" << path_sqlite << "');";
		if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
			logger::debug(LOGGER_P1,"#2 ",sqlite3_errmsg(sqlite3_DB));
		}
		sqlite3_free(path_sqlite);
	}
}

void DB_share::add_entry_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	add_entry_entry_exists = true;
}

void DB_share::delete_hash(const std::string & hash)
{
	std::ostringstream query;
	query << "DELETE FROM share WHERE hash = \"" << hash << "\";";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,"#1 ",sqlite3_errmsg(sqlite3_DB));
	}
}

bool DB_share::file_exists(const std::string & path, std::string & existing_hash, uint64_t & existing_size)
{
	file_exists_cond = false;
	file_exists_hash_ptr = &existing_hash;
	file_exists_size_ptr = &existing_size;

	char * query = sqlite3_mprintf("SELECT hash, size FROM share WHERE path = \"%q\" LIMIT 1;", path.c_str());
	if(sqlite3_exec(sqlite3_DB, query, file_exists_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	sqlite3_free(query);
	return file_exists_cond;
}

void DB_share::file_exists_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	file_exists_cond = true;
	*file_exists_hash_ptr = query_response[0];
	*file_exists_size_ptr = strtoull(query_response[1], NULL, 10);
}

bool DB_share::file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path)
{
	boost::mutex::scoped_lock lock(Mutex);

	file_info_entry_exists = false;

	//locate the record
	std::ostringstream query;
	query << "SELECT size, path FROM share WHERE ID LIKE \"" << file_ID << "\" LIMIT 1;";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), file_info_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
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

void DB_share::file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	file_info_entry_exists = true;
	file_info_file_size = strtoull(query_response[0], NULL, 10);
	file_info_file_path.assign(query_response[1]);
}

void DB_share::remove_missing()
{
	if(sqlite3_exec(sqlite3_DB, "SELECT hash, path FROM share;", remove_missing_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_share::remove_missing_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	boost::mutex::scoped_lock lock(Mutex);

	usleep(1);
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
