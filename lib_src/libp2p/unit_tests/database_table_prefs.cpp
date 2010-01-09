//custom
#include "../database.hpp"

int fail(0);

int main()
{
	//setup database, there is no need to clear this table for testing
	path::override_database_name("database_table_prefs.db");
	path::override_program_directory("");
	database::init::drop_all();
	database::init::create_all();

	//test set/get of max_download_rate
	database::table::prefs::set_max_download_rate(123);
	if(database::table::prefs::get_max_download_rate() != 123){
		LOGGER; ++fail;
	}

	//test set/get of max_connections
	database::table::prefs::set_max_connections(123);
	if(database::table::prefs::get_max_connections() != 123){
		LOGGER; ++fail;
	}

	//test set/get of max_upload_rate
	database::table::prefs::set_max_upload_rate(123);
	if(database::table::prefs::get_max_upload_rate() != 123){
		LOGGER; ++fail;
	}
	return fail;
}
