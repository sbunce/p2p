//custom
#include "../database.hpp"

int fail(0);

int main()
{
	//setup database and make sure blacklist table clear
	path::test_override("database_table_peer.db");
	database::init::drop_all();
	database::init::create_all();


	return fail;
}
