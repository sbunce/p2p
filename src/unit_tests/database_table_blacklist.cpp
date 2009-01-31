//custom
#include "../database.hpp"
#include "../global.hpp"

int main()
{
	database::connection DB;
	DB.query("DROP TABLE IF EXISTS blacklist");

	database::table::blacklist DBB;
	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(DBB.modified(state)){
		LOGGER; exit(1);
	}

	DBB.add(IP_1);

	if(!DBB.modified(state)){
		LOGGER; exit(1);
	}

	if(!DBB.is_blacklisted(IP_1)){
		LOGGER; exit(1);
	}

	if(DBB.is_blacklisted(IP_2)){
		LOGGER; exit(1);
	}

	return 0;
}
