//custom
#include "../database.hpp"

int main()
{
	//setup database and make sure blacklist table clear
	path::unit_test_override("database_table_host.db");
	database::init::drop_all();
	database::init::create_all();

	std::string test_hash = "ABC";
	database::table::host::host_info test_HI_0("123", "456");
	database::table::host::host_info test_HI_1("321", "654");

	//table empty, should not be able to look up hosts
	if(boost::shared_ptr<std::vector<database::table::host::host_info> >
		H = database::table::host::lookup(test_hash))
	{
		LOGGER; exit(1);
	}

	//add the same record twice (it should then only exist in table once)
	database::table::host::add(test_hash, test_HI_0);
	database::table::host::add(test_hash, test_HI_0);

	//look up all hosts for test_hash (there should be 1)
	if(boost::shared_ptr<std::vector<database::table::host::host_info> >
		H = database::table::host::lookup(test_hash))
	{
		if(H->size() != 1){
			LOGGER; exit(1);
		}
		if(*H->begin() != test_HI_0){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//insert new host for hash
	database::table::host::add(test_hash, test_HI_1);

	//look up all hosts for test_hash (there should be 2)
	if(boost::shared_ptr<std::vector<database::table::host::host_info> >
		H = database::table::host::lookup(test_hash))
	{
		if(H->size() != 2){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}
}
