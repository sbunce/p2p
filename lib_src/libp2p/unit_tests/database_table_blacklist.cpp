//custom
#include "../database.hpp"

int main()
{
	database::table::blacklist::clear();
	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";
	int state = 0;
	if(database::table::blacklist::modified(state)){
		LOGGER; exit(1);
	}
	database::table::blacklist::add(IP_1);
	if(!database::table::blacklist::modified(state)){
		LOGGER; exit(1);
	}
	if(!database::table::blacklist::is_blacklisted(IP_1)){
		LOGGER; exit(1);
	}
	if(database::table::blacklist::is_blacklisted(IP_2)){
		LOGGER; exit(1);
	}
}
