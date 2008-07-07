#include "DB_blacklist.h"

DB_blacklist * DB_blacklist::DB_Blacklist = NULL;
boost::mutex DB_blacklist::init_mutex;
volatile int DB_blacklist::blacklist_state = 0;

DB_blacklist::DB_blacklist()
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS blacklist_index ON blacklist (IP)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_blacklist::add_worker(const std::string & IP)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::ostringstream query;
	query << "INSERT INTO blacklist VALUES ('" << IP << "')";
	sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL);
}

bool DB_blacklist::is_blacklisted_worker(const std::string & IP)
{
	boost::mutex::scoped_lock lock(Mutex);
	bool found = false;
	std::ostringstream query;
	query << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), is_blacklisted_call_back, &found, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	return found;
}
