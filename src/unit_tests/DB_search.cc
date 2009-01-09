//custom
#include "../DB_search.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("DROP TABLE IF EXISTS search");

	DB_search DBS;

	/*
	There is currently no function to insert in to database. This makes unit test
	impossible. However, an insert function will eventually be needed.
	*/

	return 0;
}
