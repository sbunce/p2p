//THREADSAFE
#ifndef H_DATABASE_INIT
#define H_DATABASE_INIT

//boost
#include <boost/thread.hpp>

//custom
#include "global.hpp"

//include
#include <singleton.hpp>

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
	init(){}

	/*
	Recursive mutex needed because database::connection will call run, and run()
	will create a database::connection which will call run().
	*/
	static boost::once_flag once_flag;
	static void init_statics()
	{
		_Recursive_Mutex = new boost::recursive_mutex();
		_program_start = new bool(true);
	}
	//do not use this directly, use the accessor function
	static boost::recursive_mutex * _Recursive_Mutex;
	static bool * _program_start;

	//accessor functions
	static boost::recursive_mutex & Recursive_Mutex()
	{
		boost::call_once(init_statics, once_flag);
		return *_Recursive_Mutex;
	}
	static bool & program_start()
	{
		boost::call_once(init_statics, once_flag);
		return *_program_start;
	}


};
}//end of database namespace
#endif
