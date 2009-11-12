//custom
#include "../database.hpp"

int fail(0);

int main()
{
	//setup database and make sure blacklist table clear
	path::unit_test_override("database_table_host.db");
	database::init::drop_all();
	database::init::create_all();

	std::string hash = "ABC";
	std::pair<std::string, std::string> host_0("123", "456");
	std::pair<std::string, std::string> host_1("321", "654");

	std::set<std::pair<std::string, std::string> > all;

	//table empty, should not be able to look up hosts
	all = database::table::host::lookup(hash);
	if(!all.empty()){
		LOGGER; ++fail;
	}

	//add the same record twice (it should then only exist in table once)
	database::table::host::add(hash, host_0);
	database::table::host::add(hash, host_0);

	//look up all hosts for test_hash (there should be 1)
	all = database::table::host::lookup(hash);
	if(all.size() != 1){
		LOGGER; ++fail;
	}

	//insert new host for hash
	database::table::host::add(hash, host_1);

	//look up all hosts for test_hash (there should be 2)
	all = database::table::host::lookup(hash);
	if(all.size() != 2){
		LOGGER; ++fail;
	}
	return fail;
}
