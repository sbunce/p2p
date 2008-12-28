#ifndef H_DB_HASH_FREE
#define H_DB_HASH_FREE

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

//std
#include <string>

class DB_hash_free
{
public:
	DB_hash_free();

	/*

	*/


private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

};
#endif
