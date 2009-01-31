#include "database_table_prime.hpp"

boost::mutex database::table::prime::program_start_mutex;
bool database::table::prime::program_start(true);
atomic_int<unsigned> database::table::prime::prime_count(0);

static int prime_count_call_back(atomic_int<unsigned> & prime_count, int columns_retrieved,
	char ** response, char ** column_names)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> prime_count;
	return 0;
}

database::table::prime::prime()
{
	DB.query("CREATE TABLE IF NOT EXISTS prime (key INTEGER PRIMARY KEY, number TEXT)");
	{
	boost::mutex::scoped_lock lock(program_start_mutex);
	if(program_start){
		program_start = false;
		DB.query("SELECT count(1) FROM prime", &prime_count_call_back, prime_count);
	}
	}
}

void database::table::prime::add(mpint & prime)
{
	std::stringstream ss;
	ss << "INSERT INTO prime VALUES (NULL,'" << prime.to_str(64) << "')";
	DB.query(ss.str());
	++prime_count;
}

unsigned database::table::prime::count()
{
	return prime_count;
}

//std::pair<true if found, prime>
int database::table::prime::retrieve_call_back(std::pair<bool, mpint *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.first = true;
	*info.second = mpint(response[1], 64);
	std::stringstream ss;
	ss << "DELETE FROM prime WHERE key = " << response[0];
	DB.query(ss.str());
	--prime_count;
	return 0;
}

bool database::table::prime::retrieve(mpint & prime)
{
	std::pair<bool, mpint *> info(false, &prime);
	DB.query("SELECT key, number FROM prime LIMIT 1", this, &database::table::prime::retrieve_call_back, info);
	return info.first;
}
