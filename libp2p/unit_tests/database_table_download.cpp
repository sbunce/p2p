//custom
#include "../database.hpp"
#include "../path.hpp"
#include "../settings.hpp"

//include
#include <logger.hpp>

//std
#include <vector>

int main()
{
	database::connection DB("DB");
	database::init::run("DB");
	DB.query("DELETE FROM download");

	download_info DI("ABC", "NAME", 123);

	if(!database::table::download::start(DI, DB)){
		LOGGER; exit(1);
	}

	if(!database::table::download::start(DI, DB)){
		LOGGER; exit(1);
	}

	boost::int64_t key;
	if(!database::table::download::lookup_hash("ABC", DB)){
		LOGGER; exit(1);
	}

	std::string path;
	if(database::table::download::lookup_hash("ABC", path, DB)){
		if(path != path::download_unfinished() + "NAME"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	path = "";
	boost::uint64_t file_size = 0;
	if(database::table::download::lookup_hash("ABC", path, file_size, DB)){
		if(file_size != 123){
			LOGGER; exit(1);
		}
		if(path != path::download_unfinished()+"NAME"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	std::vector<download_info> resume;
	database::table::download::resume(resume, DB);

	if(resume.size() != 1){std::cout<< "size: " << resume.size() << "\n"; LOGGER; exit(1); }
	if(resume.front().hash != "ABC"){ LOGGER; exit(1); }
	if(resume.front().name != "NAME"){ LOGGER; exit(1); }
	if(resume.front().size != 123){ LOGGER; exit(1); }
}
