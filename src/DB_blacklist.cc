#include "DB_blacklist.h"

volatile DB_blacklist * DB_blacklist::DB_Blacklist = NULL;
boost::mutex DB_blacklist::init_mutex;

DB_blacklist::DB_blacklist(
):
	last_time_added(time(NULL))
{
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS blacklist (host TEXT UNIQUE)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS blacklist_index ON blacklist (host)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_blacklist::add_(const std::string & host)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::ostringstream query;
	query << "INSERT INTO blacklist VALUES ('" << host << "')";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	last_time_added = time(NULL);
}

bool DB_blacklist::is_blacklisted_(const std::string & host)
{
	boost::mutex::scoped_lock lock(Mutex);
	bool found = false;
	std::ostringstream query;
	query << "SELECT 1 FROM blacklist WHERE host = '" << host << "'";
	if(sqlite3_exec(sqlite3_DB, query.str().c_str(), is_blacklisted_call_back, &found, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

time_t DB_blacklist::last_added_()
{
	boost::mutex::scoped_lock lock(Mutex);
	return last_time_added;
}
