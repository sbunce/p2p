//custom
#include "../database.h"
#include "../DB_share.h"
#include "../global.h"

int main()
{
	database DB;
	DB.query("DROP TABLE IF EXISTS share");
	DB_share DBS;

	if(DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	DBS.add_entry("ABC", 123, "DEF");
	if(!DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	DBS.delete_entry("DEF");
	if(DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	DBS.add_entry("ABC", 123, "DEF");
	if(!DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	std::string path;
	if(DBS.lookup_hash("ABC", path)){
		if(path != "DEF"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	boost::uint64_t size = 0;
	if(DBS.lookup_hash("ABC", size)){
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	size = 0;
	if(DBS.lookup_hash("ABC", path, size)){
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
	if(DBS.lookup_path("DEF", hash, size)){
		if(hash != "ABC"){
			LOGGER; exit(1);
		}
		if(size != 123){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	return 0;
}
