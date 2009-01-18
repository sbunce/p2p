//custom
#include "../DB_share.h"
#include "../global.h"

int main()
{
	sqlite3_wrapper::database DB;
	DB.query("DROP TABLE IF EXISTS share");
	DB_share DBS;

	if(DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	DBS.add_entry("ABC", 1, 123, "DEF");
	if(!DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	DBS.delete_entry(1, "DEF");
	if(DBS.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	DBS.add_entry("ABC", 1, 123, "DEF");
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
	boost::int64_t key;
	if(DBS.lookup_path("DEF", key, size)){
		if(key != 1){
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
