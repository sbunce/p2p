//custom
#include "../db_all.hpp"

//include
#include <unit_test.hpp>

int fail(0);

int main()
{
	unit_test::timeout();

	//setup database, there is no need to clear this table for testing
	path::set_db_file_name("database_table_prefs.db");
	path::set_program_dir("");
	db::init::drop_all();
	db::init::create_all();

	//max_download_rate
	db::table::prefs::set_max_download_rate(123);
	if(db::table::prefs::get_max_download_rate() != 123){
		LOG; ++fail;
	}

	//max_connections
	db::table::prefs::set_max_connections(123);
	if(db::table::prefs::get_max_connections() != 123){
		LOG; ++fail;
	}

	//max_upload_rate
	db::table::prefs::set_max_upload_rate(123);
	if(db::table::prefs::get_max_upload_rate() != 123){
		LOG; ++fail;
	}

	//port
	db::table::prefs::set_port("1234");
	if(db::table::prefs::get_port() != "1234"){
		LOG; ++fail;
	}

	return fail;
}
