#ifndef H_SINGLETON_START
#define H_SINGLETON_START

//custom
#include "database.hpp"
#include "thread_pool.hpp"

class singleton_start
{
public:
	singleton_start()
	{
		/*
		The order these are specified in is important. The C++ standard specifies
		that order of destruction of static objects is the opposite of order of
		initialization. The singletons are static objects which get initialized
		when we first call the singleton() member function. We specify the order
		of initialization (and consequently destruction) based on their
		dependencies on other static objects.
		*/
		database::pool::singleton();  //pool needed for all database operations
		database::init Database_Init; //create tables after pool up
		thread_pool::singleton();

		/*
		Start singleton threads. This is not done in the ctors because the
		singleton threads use other singletons. We don't want to introduce a race
		condition where one singleton is allowed to trigger the construction of
		another in the wrong order.
		*/
		thread_pool::singleton().start();
	}
};
#endif
