//THREADSAFE
#ifndef H_DATABASE_INIT
#define H_DATABASE_INIT

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <database_connection.hpp>

namespace database{
class init
{
public:
	init();

	//setup all database tables
	static void run(const std::string & database_path);

private:

	/*
	The function name corresponds to the table it will setup.
	*/
	static void blacklist(database::connection & DB);
	static void download(database::connection & DB);
	static void hash(database::connection & DB);
	static void preferences(database::connection & DB);
	static void prime(database::connection & DB);
	static void search(database::connection & DB);
	static void share(database::connection & DB);
};
}//end of database namespace
#endif
