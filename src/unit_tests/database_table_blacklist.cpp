//custom
#include "../database.hpp"
#include "../settings.hpp"

int main()
{
	database::init::run();
	database::connection DB;
	DB.query("DELETE FROM blacklist");
	database::table::blacklist DBB;

	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(DBB.modified(state)){
		LOGGER; exit(1);
	}

	DBB.add(IP_1, DB);

	if(!DBB.modified(state)){
		LOGGER; exit(1);
	}

	if(!DBB.is_blacklisted(IP_1, DB)){
		LOGGER; exit(1);
	}

	if(DBB.is_blacklisted(IP_2, DB)){
		LOGGER; exit(1);
	}

	return 0;
}
