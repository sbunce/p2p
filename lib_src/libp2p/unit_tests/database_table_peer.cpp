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

	return fail;
}
