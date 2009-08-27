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
	create_all:
		Create tables that don't yet exist.
	drop_all:
		Drop all tables if they exist.
	*/
	static void create_all();
	static void drop_all();

	//functions to create tables
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
