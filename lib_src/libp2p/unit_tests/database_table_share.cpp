//custom
#include "../database.hpp"

int main()
{
	//setup database and make sure share table clear
	path::unit_test_override("database_table_share.db");
	database::init::drop_all();
	database::init::create_all();

	//test file info
	database::table::share::file_info FI;
	FI.hash = "ABC";
	FI.file_size = 123;
	FI.path = "DEF";

	//file not yet added, lookups shouldn't work
	if(database::table::share::lookup_hash(FI.hash)){
		LOGGER; exit(1);
	}
	if(database::table::share::lookup_path(FI.path)){
		LOGGER; exit(1);
	}

	//add file
	database::table::share::add_entry(FI);

	//make sure lookups work
	if(boost::shared_ptr<database::table::share::file_info>
		FI_ptr = database::table::share::lookup_hash(FI.hash))
	{
		if(FI_ptr->hash != FI.hash){ LOGGER; exit(1); }
		if(FI_ptr->file_size != FI.file_size){ LOGGER; exit(1); }
		if(FI_ptr->path != FI.path){ LOGGER; exit(1); }
	}else{
		LOGGER; exit(1);
	}
	if(boost::shared_ptr<database::table::share::file_info>
		FI_ptr = database::table::share::lookup_path(FI.path))
	{
		if(FI_ptr->hash != FI.hash){ LOGGER; exit(1); }
		if(FI_ptr->file_size != FI.file_size){ LOGGER; exit(1); }
		if(FI_ptr->path != FI.path){ LOGGER; exit(1); }
	}else{
		LOGGER; exit(1);
	}

	//delete file
	database::table::share::delete_entry(FI.path);

	//file was deleted, make sure lookups don't work
	if(database::table::share::lookup_hash(FI.hash)){
		LOGGER; exit(1);
	}
	if(database::table::share::lookup_path(FI.path)){
		LOGGER; exit(1);
	}
}
