//THREADSAFE
#ifndef H_DATABASE_INIT
#define H_DATABASE_INIT

//custom
#include "global.hpp"

namespace database{
class init
{
public:
	/*
	Sets up all tables in the database. This can be called multiple times and it
	will only do the setup the first time it is called.
	*/
	static void run();

private:
	//will never be instantiated, class used to hide functions
	init(){}
};
}//end of database namespace
#endif
