#include "DB_prime.h"

atomic_bool DB_prime::program_start(true);
atomic_int<unsigned> DB_prime::prime_count(0);

static int prime_count_call_back(atomic_int<unsigned> & prime_count, int columns_retrieved,
	char ** response, char ** column_names)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> prime_count;
	return 0;
}

DB_prime::DB_prime()
{
	DB.query("CREATE TABLE IF NOT EXISTS prime (key INTEGER PRIMARY KEY, number TEXT)");
	if(program_start){
		DB.query("SELECT count(1) FROM prime", &prime_count_call_back, prime_count);
		program_start = false;
	}
}

void DB_prime::add(mpint & prime)
{
	std::ostringstream query;
	query << "INSERT INTO prime VALUES (NULL,'" << prime.to_str(64) << "')";
	DB.query(query.str());
	++prime_count;
}

int DB_prime::count()
{
	return prime_count;
}

//std::pair<true if found, prime>
int DB_prime::retrieve_call_back(std::pair<bool, mpint *> & info, int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.first = true;
	*info.second = mpint(response[1], 64);
	std::ostringstream query;
	query << "DELETE FROM prime WHERE key = " << response[0];
	DB.query(query.str());
	--prime_count;
	return 0;
}

bool DB_prime::retrieve(mpint & prime)
{
	std::pair<bool, mpint *> info(false, &prime);
	DB.query("SELECT key, number FROM prime LIMIT 1", this, &DB_prime::retrieve_call_back, info);
	return info.first;
}
