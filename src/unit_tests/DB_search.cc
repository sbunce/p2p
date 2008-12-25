//custom
#include "../DB_search.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	//initial tests fail without this
	std::remove(global::DATABASE_PATH.c_str());

	DB_search DBS;

	/*
	There is currently no function to insert in to database. This makes unit test
	impossible. However, an insert function will eventually be needed.
	*/

	return 0;
}
