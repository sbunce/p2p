//THREADSAFE
#ifndef H_DATABASE_TABLE_PRIME
#define H_DATABASE_TABLE_PRIME

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/tuple/tuple.hpp>
#include <tommath/mpint.hpp>

//std
#include <iostream>
#include <sstream>

namespace database{
namespace table{
class prime
{
public:
	/*
	read_all:
		Copies all primes in database to vector and clears prime table.
	write_all:
		Copies all primes in vector to database.
	*/
	static void read_all(std::vector<mpint> & Prime_Cache);
	static void write_all(std::vector<mpint> & Prime_Cache);

	/* Connection Functions
	All these functions correspond to above functions but they take a database
	connection. These are needed to do calls to multiple database functions
	within a transaction.
	*/
	static void read_all(std::vector<mpint> & Prime_Cache, database::pool::proxy & DB);
	static void write_all(std::vector<mpint> & Prime_Cache, database::pool::proxy & DB);

private:
	prime(){}
};
}//end of namespace table
}//end of namespace database
#endif
