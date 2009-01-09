//custom
#include "../DB_share.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("DROP TABLE IF EXISTS share");

	DB_share DBS;

	if(DBS.lookup_hash("ABC")){
		std::cout << "failed lookup hash 1 test\n";
		return 1;
	}

	DBS.add_entry("ABC", 123, "DEF");

	if(!DBS.lookup_hash("ABC")){
		std::cout << "failed lookup hash 2 test\n";
		return 1;
	}

	DBS.delete_hash("ABC", "DEF");

	if(DBS.lookup_hash("ABC")){
		std::cout << "failed lookup hash 3 test\n";
		return 1;
	}

	DBS.add_entry("ABC", 123, "DEF");

	if(!DBS.lookup_hash("ABC")){
		std::cout << "failed lookup hash 4 test\n";
		return 1;
	}

	std::string path;
	if(DBS.lookup_hash("ABC", path)){
		if(path != "DEF"){
			std::cout << "failed lookup hash 5 test\n";
			return 1;
		}
	}else{
		std::cout << "failed lookup hash 6 test\n";
		return 1;
	}

	boost::uint64_t size = 0;
	if(DBS.lookup_hash("ABC", path, size)){
		if(path != "DEF"){
			std::cout << "failed lookup hash 7 test\n";
			return 1;
		}
		if(size != 123){
			std::cout << "failed lookup hash 8 test\n";
			return 1;
		}
	}else{
		std::cout << "failed lookup hash 9 test\n";
		return 1;
	}

	size = 0;
	std::string hash;
	if(DBS.lookup_path("DEF", hash, size)){
		if(hash != "ABC"){
			std::cout << "failed lookup hash 10 test\n";
			return 1;
		}
		if(size != 123){
			std::cout << "failed lookup hash 11 test\n";
			return 1;
		}
	}else{
		std::cout << "failed lookup hash 12 test\n";
		return 1;
	}

	//remove missing not tested

	return 0;
}
