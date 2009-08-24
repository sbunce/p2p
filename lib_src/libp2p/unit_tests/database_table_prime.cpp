//custom
#include "../database.hpp"

int main()
{
	/*
	On program start all primes are read from the database in to a vector and
	then deleted from the database (in a transaction). On program shutdown all
	primes are written to the database.
	*/

	//setup database and make sure prime table clear
	path::unit_test_override("database_table_prime.db");
	database::init::drop_all();
	database::init::create_all();

	//vector of primes to test
	std::vector<mpint> Prime_Cache;
	Prime_Cache.push_back(mpint("2"));

	//write all primes to database and clear our vector
	database::table::prime::write_all(Prime_Cache);
	Prime_Cache.clear();

	//read all primes in to vector and check to make sure it's the same
	database::table::prime::read_all(Prime_Cache);
	if(Prime_Cache.size() != 1){
		LOGGER; exit(1);
	}
	if(Prime_Cache.front() != 2){
		LOGGER; exit(1);
	}
}
