//custom
#include "../database.hpp"

int fail(0);

int main()
{
	//setup database and make sure blacklist table clear
	path::override_database_name("database_table_peer.db");
	path::override_program_directory("");
	database::init::drop_all();
	database::init::create_all();

	database::table::peer::info info(
		"DEADBEEFDEADBEEFDEAD",
		"123.123.123.123",
		"32768"
	);

	//add
	database::table::peer::add(info);

	//verify
	std::list<database::table::peer::info> resume = database::table::peer::resume();
	if(resume.size() != 1){
		LOGGER; ++fail;
	}else{
		if(resume.front().ID != "DEADBEEFDEADBEEFDEAD"){
			LOGGER; ++fail;
		}
		if(resume.front().IP != "123.123.123.123"){
			LOGGER; ++fail;
		}
		if(resume.front().port != "32768"){
			LOGGER; ++fail;
		}
	}

	//remove
	database::table::peer::remove(info.ID);
	resume = database::table::peer::resume();
	if(!resume.empty()){
		LOGGER; ++fail;
	}

	return fail;
}
