//custom
#include "../database.hpp"

int main()
{
	database::table::prime::clear();
	std::vector<mpint> Prime_Cache;
	Prime_Cache.push_back(mpint("123"));
	database::table::prime::write_all(Prime_Cache);
	Prime_Cache.clear();
	database::table::prime::read_all(Prime_Cache);
	if(Prime_Cache.size() != 1){
		LOGGER; exit(1);
	}
	if(Prime_Cache.front() != 123){
		LOGGER; exit(1);
	}
}
