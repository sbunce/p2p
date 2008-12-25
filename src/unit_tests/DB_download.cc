//custom
#include "../DB_download.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	//initial tests fail without this
	std::remove(global::DATABASE_PATH.c_str());

	DB_download DBD;
	download_info DI("ABC", "NAME", 123);

	if(!DBD.start_download(DI)){
		std::cout << "failed start download 1 test\n";
		return 1;
	}

	if(DBD.start_download(DI)){
		std::cout << "failed start download 2 test\n";
		return 1;
	}

	boost::uint64_t file_size;
	if(DBD.lookup_hash("ABC", file_size)){
		if(file_size != 123){
			std::cout << "failed lookup hash 1 test, bad file size\n";
			return 1;
		}
	}else{
		std::cout << "failed lookup hash 1 test, lookup failure\n";
		return 1;
	}

	std::string path;
	if(DBD.lookup_hash("ABC", path)){
		if(path != global::DOWNLOAD_DIRECTORY+"NAME"){
			std::cout << "failed lookup hash 2 test, bad path\n";
			return 1;
		}
	}else{
		std::cout << "failed lookup hash 2 test, lookup failure\n";
		return 1;
	}

	path = "";
	file_size = 0;
	if(DBD.lookup_hash("ABC", path, file_size)){
		if(file_size != 123){
			std::cout << "failed lookup hash 3 test, bad file size\n";
			return 1;
		}
		if(path != global::DOWNLOAD_DIRECTORY+"NAME"){
			std::cout << "failed lookup hash 3 test, bad path\n";
			return 1;
		}
	}else{
		std::cout << "failed lookup hash 3 test, lookup failure\n";
		return 1;
	}

	//DBD::resume is not tested

	return 0;
}
