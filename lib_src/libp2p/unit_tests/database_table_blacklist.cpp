//custom
#include "../database.hpp"

int fail(0);

int main()
{
	//setup database and make sure blacklist table clear
	path::override_database_name("database_table_blacklist.db");
	path::override_program_directory("");
	database::init::drop_all();
	database::init::create_all();

	/*
	The default blacklist state is 0. The blacklist state tells us if the
	blacklist has been modified. If the blacklist has been modified it tells the
	user of the blacklist that they need to check who's currently connected to
	see if anyone has been blacklisted.
	*/
	int state = 0;

	//the blacklist should not be modified by default
	if(database::table::blacklist::modified(state)){
		LOGGER; ++fail;
	}

	//after adding an IP the blacklist should be modified
	database::table::blacklist::add("1.1.1.1");
	if(!database::table::blacklist::modified(state)){
		LOGGER; ++fail;
	}

	//the IP we added should be now blacklisted
	if(!database::table::blacklist::is_blacklisted("1.1.1.1")){
		LOGGER; ++fail;
	}

	//we didn't add this IP, it shouldn't be blacklisted
	if(database::table::blacklist::is_blacklisted("2.2.2.2")){
		LOGGER; ++fail;
	}
	return fail;
}
