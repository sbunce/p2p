#include "database_table_prime.hpp"

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
	DB->query("BEGIN TRANSACTION");
	DB->query("SELECT number FROM prime", &read_all_call_back, Prime_Cache);
	DB->query("DELETE FROM prime");
	DB->query("END TRANSACTION");
}

void database::table::prime::write_all(std::vector<mpint> & Prime_Cache,
	database::pool::proxy DB)
{
	std::stringstream ss;
	for(int x=0; x<Prime_Cache.size(); ++x){
		ss << "INSERT INTO prime VALUES ('" << Prime_Cache[x].to_str(64) << "');";
	}
	DB->query(ss.str());
}
