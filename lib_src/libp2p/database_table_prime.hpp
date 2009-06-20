/*
This should generally only be used by the number_generator class.
*/
//THREADSAFE
#ifndef H_DATABASE_TABLE_PRIME
#define H_DATABASE_TABLE_PRIME

//boost
#include <boost/tuple/tuple.hpp>

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <database_connection.hpp>

//std
#include <iostream>
#include <sstream>

//tommath
#include <tommath/mpint.hpp>

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
	static void read_all(std::vector<mpint> & Prime_Cache, database::connection & DB);
	static void write_all(std::vector<mpint> & Prime_Cache, database::connection & DB);

private:
	prime(){}
};
}//end of table namespace
}//end of database namespace
#endif
