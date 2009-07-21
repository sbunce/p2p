//custom
#include "../database.hpp"

int main()
{
	//setup database and make sure blacklist table clear
	database::pool::singleton().unit_test_override("database_table_blacklist.db");
	database::init init;
	database::table::blacklist::clear();

	/*
	The default blacklist state is 0. The blacklist state tells us if the
	blacklist has been modified. If the blacklist has been modified it tells the
	user of the blacklist that they need to check who's currently connected to
	see if anyone has been blacklisted.
	*/
	int state = 0;

	//the blacklist should not be modified by default
	if(database::table::blacklist::modified(state)){
		LOGGER; exit(1);
	}

	//after adding an IP the blacklist should be modified
	database::table::blacklist::add("1.1.1.1");
	if(!database::table::blacklist::modified(state)){
		LOGGER; exit(1);
	}

	//the IP we added should be now blacklisted
	if(!database::table::blacklist::is_blacklisted("1.1.1.1")){
		LOGGER; exit(1);
	}

	//we didn't add this IP, it shouldn't be blacklisted
	if(database::table::blacklist::is_blacklisted("2.2.2.2")){
		LOGGER; exit(1);
	}
}
