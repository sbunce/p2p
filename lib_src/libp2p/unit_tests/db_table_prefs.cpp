//custom
#include "../db_all.hpp"

int fail(0);

int main()
{
	//setup database, there is no need to clear this table for testing
	path::override_database_name("database_table_prefs.db");
	path::override_program_directory("");
	db::init::drop_all();
	db::init::create_all();

	//test set/get of max_download_rate
	db::table::prefs::set_max_download_rate(123);
	if(db::table::prefs::get_max_download_rate() != 123){
		LOG; ++fail;
	}

	//test set/get of max_connections
	db::table::prefs::set_max_connections(123);
	if(db::table::prefs::get_max_connections() != 123){
		LOG; ++fail;
	}

	//test set/get of max_upload_rate
	db::table::prefs::set_max_upload_rate(123);
	if(db::table::prefs::get_max_upload_rate() != 123){
		LOG; ++fail;
	}
	return fail;
}
