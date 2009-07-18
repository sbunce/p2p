#include "database_table_prime.hpp"

boost::once_flag database::table::prime::once_flag = BOOST_ONCE_INIT;

void database::table::prime::once_func(database::pool::proxy & DB)
{
	DB->query("CREATE TABLE IF NOT EXISTS prime (number TEXT)");
}

int read_all_call_back(std::vector<mpint> & Prime_Cache,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	Prime_Cache.push_back(mpint(response[0], 64));
	return 0;
}

void database::table::prime::read_all(std::vector<mpint> & Prime_Cache,
	database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	DB->query("BEGIN TRANSACTION");
	DB->query("SELECT number FROM prime", &read_all_call_back, Prime_Cache);
	DB->query("DELETE FROM prime");
	DB->query("END TRANSACTION");
}

void database::table::prime::write_all(std::vector<mpint> & Prime_Cache,
	database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	std::stringstream ss;
	for(int x=0; x<Prime_Cache.size(); ++x){
		ss << "INSERT INTO prime VALUES ('" << Prime_Cache[x].to_str(64) << "');";
	}
	DB->query(ss.str());
}
