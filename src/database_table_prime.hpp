//THREADSAFE
#ifndef H_DATABASE_TABLE_PRIME
#define H_DATABASE_TABLE_PRIME

//boost
#include <boost/tuple/tuple.hpp>

//custom
#include "database_connection.hpp"
#include "settings.hpp"

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
	prime(){}

	/*
	add      - add a prime to the DB
	count    - returns how many primes are in the database
	retrieve - retrieve a prime from the DB and delete the prime from the DB
	           returns true if prime retrieved, false if prime table empty
	*/
	void add(mpint & prime);
	static void add(mpint & prime, database::connection & DB);
	static unsigned count();
	bool retrieve(mpint & prime);
	static bool retrieve(mpint & prime, database::connection & DB);

private:
	database::connection DB;

	static boost::once_flag once_flag;
	static void init()
	{
		_Mutex = new boost::mutex();
		_prime_count = new unsigned(0);
	}
	//do not use this directly, use the accessor function
	static boost::mutex * _Mutex;
	static unsigned * _prime_count;

	//accessor functions
	static boost::mutex & Mutex()
	{
		boost::call_once(init, once_flag);
		return *_Mutex;
	}
	static unsigned & prime_count()
	{
		boost::call_once(init, once_flag);
		return *_prime_count;
	}

	/*
	initialize_prime_count - retrieves prime count from database if not already done
	                         Note: must be called by all public member functions
	                         WARNING: Mutex must be locked before calling this
	retrieve_call_back     - call back for retrieve()
	*/
	static void initialize_prime_count();
	static int retrieve_call_back(boost::tuple<bool, mpint *, database::connection *> & info,
		int columns_retrieved, char ** response, char ** column_name);
};
}//end of table namespace
}//end of database namespace
#endif
