#ifndef H_DATABASE_INIT
#define H_DATABASE_INIT

//custom
#include "database.hpp"

//standard
#include <sstream>

namespace database{
class init
{
public:

	/*
	all:
		Set up all tables.
	<*>:
		Set up <*> table.
	*/
	static void all();
	static void blacklist();
	static void hash();
	static void preferences();
	static void prime();
	static void share();

private:
	init(){}
};
}//end of namespace database
#endif
