//custom
#include "../database.hpp"
#include "../settings.hpp"

int main()
{
	database::connection DB("DB");
	database::init::run("DB");
	DB.query("DELETE FROM share");

	if(database::table::share::lookup_hash("ABC", DB)){
		LOGGER; exit(1);
	}

	database::table::share::add_entry("ABC", 123, "DEF", DB);
	if(!database::table::share::lookup_hash("ABC", DB)){
		LOGGER; exit(1);
	}

	database::table::share::delete_entry("DEF", DB);
	if(database::table::share::lookup_hash("ABC", DB)){
		LOGGER; exit(1);
	}

	database::table::share::add_entry("ABC", 123, "DEF", DB);
	if(!database::table::share::lookup_hash("ABC", DB)){
		LOGGER; exit(1);
	}

	std::string path;
	if(database::table::share::lookup_hash("ABC", path, DB)){
		if(path != "DEF"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	boost::uint64_t size = 0;
	if(database::table::share::lookup_hash("ABC", size, DB)){
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	size = 0;
	if(database::table::share::lookup_hash("ABC", path, size, DB)){
		if(path != "DEF"){
			LOGGER; exit(1);
		}
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	size = 0;
	std::string hash;
	if(database::table::share::lookup_path("DEF", hash, size, DB)){
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
