#include "database_table_prime.hpp"

boost::mutex & database::table::prime::Mutex()
{
	static boost::mutex * M = new boost::mutex();
	return *M;
}

unsigned & database::table::prime::prime_count()
{
	static unsigned * PC = new unsigned(0);
	return *PC;
}

void database::table::prime::add(mpint & prime)
{
	add(prime, DB);
}

void database::table::prime::add(mpint & prime, database::connection & DB)
{
	boost::mutex::scoped_lock lock(Mutex());
	initialize_prime_count();
	std::stringstream ss;
	ss << "INSERT INTO prime VALUES (NULL,'" << prime.to_str(64) << "')";
	DB.query(ss.str());
	++prime_count();
}

unsigned database::table::prime::count()
{
	boost::mutex::scoped_lock lock(Mutex());
	initialize_prime_count();
	return prime_count();
}

static int prime_count_call_back(unsigned & prime_count, int columns_retrieved,
	char ** response, char ** column_names)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> prime_count;
	return 0;
}

void database::table::prime::initialize_prime_count()
{
	static bool prime_count_initialized(false);
	if(!prime_count_initialized){
		prime_count_initialized = true;
		static database::connection DB;
		DB.query("SELECT count(1) FROM prime", &prime_count_call_back, prime_count());
	}
}

//boost::tuple<true if found, prime, DB connection for delete>
int database::table::prime::retrieve_call_back(
	boost::tuple<bool, mpint *, database::connection *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.get<0>() = true;
	*info.get<1>() = mpint(response[1], 64);
	std::stringstream ss;
	ss << "DELETE FROM prime WHERE key = " << response[0];
	info.get<2>()->query(ss.str());
	--prime_count();
	return 0;
}

bool database::table::prime::retrieve(mpint & prime)
{
	return retrieve(prime, DB);
}

bool database::table::prime::retrieve(mpint & prime, database::connection & DB)
{
	boost::mutex::scoped_lock lock(Mutex());
	initialize_prime_count();
	boost::tuple<bool, mpint *, database::connection *> info(false, &prime, &DB);
	DB.query("SELECT key, number FROM prime LIMIT 1", &retrieve_call_back, info);
	return info.get<0>();
}
