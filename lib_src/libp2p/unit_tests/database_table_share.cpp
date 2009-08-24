//custom
#include "../database.hpp"

int main()
{
	//setup database and make sure share table clear
	path::unit_test_override("database_table_share.db");
	database::init::drop_all();
	database::init::create_all();

	//file not yet added, lookup shouldn't work
	if(database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	//add file and make sure lookup by hash works
	database::table::share::add_entry("ABC", 123, "DEF");
	if(!database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	//delete file and make sure lookup by hash doesn't work
	database::table::share::delete_entry("DEF");
	if(database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	//add file back to share for further testing, make sure lookup works
	database::table::share::add_entry("ABC", 123, "DEF");
	if(!database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	//test looking up path by hash
	std::string path;
	if(database::table::share::lookup_hash("ABC", path)){
		if(path != "DEF"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//test looking up size by hash
	boost::uint64_t size = 0;
	if(database::table::share::lookup_hash("ABC", size)){
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//test looking up path and size by hash
	size = 0;
	if(database::table::share::lookup_hash("ABC", path, size)){
		if(path != "DEF"){
			LOGGER; exit(1);
		}
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//test looking up hash and size by path
	size = 0;
	std::string hash;
	if(database::table::share::lookup_path("DEF", hash, size)){
		if(hash != "ABC"){
			LOGGER; exit(1);
		}
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}
}
