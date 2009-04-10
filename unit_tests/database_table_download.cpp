//custom
#include "../database.hpp"
#include "../settings.hpp"

//std
#include <vector>

int main()
{
	database::init::run();
	database::connection DB;
	DB.query("DELETE FROM download");

	database::table::download DBD;
	download_info DI("ABC", "NAME", 123);

	if(!DBD.start(DI)){
		LOGGER; exit(1);
	}

	if(!DBD.start(DI)){
		LOGGER; exit(1);
	}

	boost::int64_t key;
	if(!DBD.lookup_hash("ABC")){
		LOGGER; exit(1);
	}

	std::string path;
	if(DBD.lookup_hash("ABC", path)){
		if(path != settings::DOWNLOAD_DIRECTORY+"NAME"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	path = "";
	boost::uint64_t file_size = 0;
	if(DBD.lookup_hash("ABC", path, file_size)){
		if(file_size != 123){
			LOGGER; exit(1);
		}
		if(path != settings::DOWNLOAD_DIRECTORY+"NAME"){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	std::vector<download_info> resume;
	DBD.resume(resume);
	if(resume.size() != 1){std::cout<< "size: " << resume.size() << "\n"; LOGGER; exit(1); }
	if(resume.front().hash != "ABC"){ LOGGER; exit(1); }
	if(resume.front().name != "NAME"){ LOGGER; exit(1); }
	if(resume.front().size != 123){ LOGGER; exit(1); }

	return 0;
}
