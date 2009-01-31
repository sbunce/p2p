#ifndef H_DATABASE_TABLE_PRIME
#define H_DATABASE_TABLE_PRIME

//custom
#include "atomic_int.hpp"
#include "database_connection.hpp"
#include "global.hpp"

//libtommath
#include <mpint.hpp>

//std
#include <iostream>
#include <sstream>

namespace database{
namespace table{
class prime
{
public:
	prime();

	/*
	add      - add a prime to the DB
	count    - returns how many primes are in the database
	retrieve - retrieve a prime from the DB and delete the prime from the DB
	           returns true if prime retrieved, false if prime table empty
	*/
	void add(mpint & prime);
	unsigned count();
	bool retrieve(mpint & prime);

private:
	database::connection DB;

	//used at program start to retrieve prime_count (see ctor)
	static boost::mutex program_start_mutex;
	static bool program_start;

	//how many primes are in the database
	static atomic_int<unsigned> prime_count;

	/*
	retrieve_call_back - call back for retrieve()
	*/
	int retrieve_call_back(std::pair<bool, mpint *> & info, int columns_retrieved, char ** response, char ** column_name);
};
}//end of table namespace
}//end of database namespace
#endif
