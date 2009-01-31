//custom
#include "../database.hpp"
#include "../global.hpp"

int main()
{
	database::connection DB;
	DB.query("DROP TABLE IF EXISTS search");

	database::table::search DBS;

	/*
	There is currently no function to insert in to database. This makes unit test
	impossible. However, an insert function will eventually be needed.
	*/

	return 0;
}
