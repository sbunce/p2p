//custom
#include "../db_all.hpp"

int fail(0);

int main()
{
	//setup database and make sure blacklist table clear
	path::set_db_file_name("database_table_peer.db");
	path::set_program_dir("");
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
	std::vector<db::table::peer::info> resume = db::table::peer::resume();
	if(resume.size() != 1){
		LOG; ++fail;
	}else{
		if(resume.front().ID != "DEADBEEFDEADBEEFDEAD"){
			LOG; ++fail;
		}
		if(resume.front().IP != "123.123.123.123"){
			LOG; ++fail;
		}
		if(resume.front().port != "32768"){
			LOG; ++fail;
		}
	}

	//remove
	db::table::peer::remove(info.ID);
	resume = db::table::peer::resume();
	if(!resume.empty()){
		LOG; ++fail;
	}

	return fail;
}
