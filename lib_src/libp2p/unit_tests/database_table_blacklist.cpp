//custom
#include "../database.hpp"
#include "../settings.hpp"

int main()
{
	database::connection DB("DB");
	database::init::run("DB");
	DB.query("DELETE FROM blacklist");

	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(database::table::blacklist::modified(state)){
		LOGGER; exit(1);
	}

	database::table::blacklist::add(IP_1, DB);

	if(!database::table::blacklist::modified(state)){
		LOGGER; exit(1);
	}

	if(!database::table::blacklist::is_blacklisted(IP_1, DB)){
		LOGGER; exit(1);
	}

	if(database::table::blacklist::is_blacklisted(IP_2, DB)){
		LOGGER; exit(1);
	}
}
