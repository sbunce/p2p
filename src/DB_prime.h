#ifndef H_DB_PRIME
#define H_DB_PRIME

//custom
#include "atomic_bool.h"
#include "atomic_int.h"
#include "global.h"
#include "sqlite3_wrapper.h"

//libtommath
#include <mpint.h>

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
	void add(mpint & prime);
	int count();
	bool retrieve(mpint & prime);

private:
	sqlite3_wrapper::database DB;

	static atomic_bool program_start;        //true when program just started
	static atomic_int<unsigned> prime_count; //how many primes are in the database

	/*
	retrieve_call_back - call back for retrieve()
	*/
	int retrieve_call_back(std::pair<bool, mpint *> & info, int columns_retrieved, char ** response, char ** column_name);
};
#endif
