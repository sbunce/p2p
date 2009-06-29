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
	static void read_all(std::vector<mpint> & Prime_Cache, database::connection & DB);
	static void write_all(std::vector<mpint> & Prime_Cache, database::connection & DB);

private:
	prime(){}
};
}//end of namespace table
}//end of namespace database
#endif
