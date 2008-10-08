#include "DB_prime.h"

boost::mutex DB_prime::Mutex;
volatile unsigned int DB_prime::prime_count(0);

static int prime_count_call_back(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_names)
{
	std::istringstream size_iss(query_response[0]);
	size_iss >> *((unsigned int *)object_ptr);
	return 0;
}

DB_prime::DB_prime()
{
	boost::mutex::scoped_lock lock(Mutex);
	//open DB
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	//DB timeout to 1 second
	if(sqlite3_busy_timeout(sqlite3_DB, 1000) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	if(sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS prime (key INTEGER PRIMARY KEY, number TEXT)", NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	//retrieve how many primes in table
	if(sqlite3_exec(sqlite3_DB, "SELECT count(1) FROM prime", prime_count_call_back, (void *)&prime_count, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void DB_prime::add(const mpint & prime)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::ostringstream oss;
	oss << "INSERT INTO prime VALUES (NULL,'" << prime << "')";
	if(sqlite3_exec(sqlite3_DB, oss.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	++prime_count;
}

unsigned int DB_prime::count()
{
	return prime_count;
}

bool DB_prime::retrieve(mpint & prime)
{
	boost::mutex::scoped_lock lock(Mutex);
	retrieve_mpint = &prime;
	retrieve_found = false;
	if(sqlite3_exec(sqlite3_DB, "SELECT key, number FROM prime LIMIT 1", retrieve_call_back_wrapper, (void *)this, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	return retrieve_found;
}

void DB_prime::retrieve_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	retrieve_found = true;
	*retrieve_mpint = mpint(query_response[1]);

	std::ostringstream oss;
	oss << "DELETE FROM prime WHERE key = " << query_response[0];
	if(sqlite3_exec(sqlite3_DB, oss.str().c_str(), NULL, NULL, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	--prime_count;
}
