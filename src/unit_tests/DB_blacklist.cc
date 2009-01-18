//custom
#include "../DB_blacklist.h"
#include "../global.h"

int main()
{
	sqlite3_wrapper::database DB;
	DB.query("DROP TABLE IF EXISTS blacklist");

	DB_blacklist DB_Blacklist;
	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(DB_Blacklist.modified(state)){
		LOGGER; exit(1);
	}

	DB_Blacklist.add(IP_1);

	if(!DB_Blacklist.modified(state)){
		LOGGER; exit(1);
	}

	if(!DB_Blacklist.is_blacklisted(IP_1)){
		LOGGER; exit(1);
	}

	if(DB_Blacklist.is_blacklisted(IP_2)){
		LOGGER; exit(1);
	}

	return 0;
}
