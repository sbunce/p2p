//custom
#include "../db_all.hpp"

//include
#include <unit_test.hpp>

int fail(0);

int main()
{
	unit_test::timeout();

	//setup database and make sure share table clear
	path::set_db_file_name("database_table_share.db");
	path::set_program_dir("");
	db::init::drop_all();
	db::init::create_all();

	//test info
	db::table::share::info SI;
	SI.hash = "ABC";
	SI.path = "/foo/bar";
	SI.file_size = 123;
	SI.last_write_time = 123;
	SI.file_state = db::table::share::downloading;

	//file not yet added, lookups shouldn't work
	if(db::table::share::find(SI.hash)){
		LOG; ++fail;
	}

	//add file
	db::table::share::add(SI);

	//make sure lookups work
	if(boost::shared_ptr<db::table::share::info>
		lookup_SI = db::table::share::find(SI.hash))
	{
		if(lookup_SI->hash != SI.hash){
			LOG; ++fail;
		}
		if(lookup_SI->path != SI.path){
			LOG; ++fail;
		}
		if(lookup_SI->file_size != SI.file_size){
			LOG; ++fail;
		}
		if(lookup_SI->last_write_time != SI.last_write_time){
			LOG; ++fail;
		}
		if(lookup_SI->file_state != SI.file_state){
			LOG; ++fail;
		}
	}else{
		LOG; ++fail;
	}

	//remove file
	db::table::share::remove(SI.path);

	//file was deleted, make sure lookups don't work
	if(db::table::share::find(SI.hash)){
		LOG; ++fail;
	}
	return fail;
}
