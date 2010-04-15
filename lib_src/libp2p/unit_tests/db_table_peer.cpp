//custom
#include "../db_all.hpp"

int fail(0);

int main()
{
	//setup database and make sure blacklist table clear
	path::override_database_name("database_table_peer.db");
	path::override_program_directory("");
	db::init::drop_all();
	db::init::create_all();

	db::table::peer::info info(
		"DEADBEEFDEADBEEFDEAD",
		"123.123.123.123",
		"32768"
	);

	//add
	db::table::peer::add(info);

	//verify
	std::list<db::table::peer::info> resume = db::table::peer::resume();
	if(resume.size() != 1){
		LOGGER(logger::utest); ++fail;
	}else{
		if(resume.front().ID != "DEADBEEFDEADBEEFDEAD"){
			LOGGER(logger::utest); ++fail;
		}
		if(resume.front().IP != "123.123.123.123"){
			LOGGER(logger::utest); ++fail;
		}
		if(resume.front().port != "32768"){
			LOGGER(logger::utest); ++fail;
		}
	}

	//remove
	db::table::peer::remove(info.ID);
	resume = db::table::peer::resume();
	if(!resume.empty()){
		LOGGER(logger::utest); ++fail;
	}

	return fail;
}
