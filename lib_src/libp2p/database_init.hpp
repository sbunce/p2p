//THREADSAFE
#ifndef H_DATABASE_INIT
#define H_DATABASE_INIT

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

namespace database{
class init
{
public:
	//setup all database tables
	static void run(const std::string & database_path);

private:
	init(){}

	/*
	The function name corresponds to the table it will setup.
	*/
	static void blacklist(database::connection & DB);
	static void hash(database::connection & DB);
	static void preferences(database::connection & DB);
	static void prime(database::connection & DB);
	static void search(database::connection & DB);
	static void share(database::connection & DB);
};
}//end of database namespace
#endif
