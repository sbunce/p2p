//custom
#include "../database.h"
#include "../database_blacklist.h"
#include "../global.h"

int main()
{
	database DB;
	DB.query("DROP TABLE IF EXISTS blacklist");

	database::blacklist DBB;
	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(DBB.modified(state)){
		LOGGER; exit(1);
	}

	DBB.add(IP_1);

	if(!DBB.modified(state)){
		LOGGER; exit(1);
	}

	if(!DBB.is_blacklisted(DB, IP_1)){
		LOGGER; exit(1);
	}

	if(DBB.is_blacklisted(DB, IP_2)){
		LOGGER; exit(1);
	}

	return 0;
}
