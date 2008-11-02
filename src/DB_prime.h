#ifndef H_DB_PRIME
#define H_DB_PRIME

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "atomic_int.h"
#include "global.h"

//libtommath
#include <mpint.h>

//sqlite
#include <sqlite3.h>

//std
#include <iostream>
#include <sstream>

class DB_prime
{
public:
	DB_prime();

	/*
	add      - add a prime to the DB
	count    - returns how many primes are in the database
	retrieve - retrieve a prime from the DB and delete the prime from the DB
	           returns true if prime retrieved, false if prime table empty
	*/
	void add(const mpint & prime);
	int count();
	bool retrieve(mpint & prime);

private:
	sqlite3 * sqlite3_DB;

	static boost::mutex Mutex;                   //mutex for all public functions
	static atomic_int<unsigned int> prime_count; //how many primes are in the database

	void retrieve_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int retrieve_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_prime * this_class = (DB_prime *)object_ptr;
		this_class->retrieve_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	bool retrieve_found;
	mpint * retrieve_mpint;
};
#endif
