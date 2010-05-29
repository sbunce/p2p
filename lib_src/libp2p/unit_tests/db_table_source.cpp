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

	std::string ID("1111111111111111111111111111111111111111");
	std::string hash("2222222222222222222222222222222222222222");
	db::table::source::add(ID, hash);

	std::list<std::string> nodes = db::table::source::get_ID(hash);
	if(nodes.size() != 1){
		LOG; ++fail;
		return fail;
	}
	if(nodes.front() != ID){
		LOG; ++fail;
	}
	return fail;
}
