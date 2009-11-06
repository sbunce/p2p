//custom
#include "../database.hpp"

int main()
{
	//setup database and make sure share table clear
	path::unit_test_override("database_table_share.db");
	database::init::drop_all();
	database::init::create_all();

	//test info
	database::table::share::info SI;
	SI.hash = "ABC";
	SI.path = "/foo/bar";
	SI.file_size = 123;
	SI.last_write_time = 123;
	SI.file_state = database::table::share::downloading;

	//file not yet added, lookups shouldn't work
	if(database::table::share::lookup(SI.hash)){
		LOGGER; exit(1);
	}

	//add file
	database::table::share::add(SI);

	//make sure lookups work
	if(boost::shared_ptr<database::table::share::info>
		lookup_SI = database::table::share::lookup(SI.hash))
	{
		if(lookup_SI->hash != SI.hash){ LOGGER; exit(1); }
		if(lookup_SI->path != SI.path){ LOGGER; exit(1); }
		if(lookup_SI->file_size != SI.file_size){ LOGGER; exit(1); }
		if(lookup_SI->last_write_time != SI.last_write_time){ LOGGER; exit(1); }
		if(lookup_SI->file_state != SI.file_state){ LOGGER; exit(1); }
	}else{
		LOGGER; exit(1);
	}

	//remove file
	database::table::share::remove(SI.path);

	//file was deleted, make sure lookups don't work
	if(database::table::share::lookup(SI.hash)){
		LOGGER; exit(1);
	}
}
