//custom
#include "../database.hpp"

int main()
{
	database::init init;
	database::table::share::clear();
	if(database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}
	database::table::share::add_entry("ABC", 123, "DEF");
	if(!database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}
	database::table::share::delete_entry("DEF");
	if(database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}
	database::table::share::add_entry("ABC", 123, "DEF");
	if(!database::table::share::lookup_hash("ABC")){
		LOGGER; exit(1);
	}
	std::string path;
	if(database::table::share::lookup_hash("ABC", path)){
		if(path != "DEF"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}
	boost::uint64_t size = 0;
	if(database::table::share::lookup_hash("ABC", size)){
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}
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
