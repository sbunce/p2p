#ifndef H_DB_BLACKLIST
#define H_DB_BLACKLIST

//boost
#include <boost/thread/mutex.hpp>

//sqlite
#include <sqlite3.h>

//std
#include <iostream>
#include <sstream>
#include <sstream>
#include <string>

//custom
#include "global.h"

class DB_blacklist
{
public:
	//add a host to the blacklist
	static void add(const std::string & host)
	{
		init();
		((DB_blacklist *)DB_Blacklist)->add_(host);
	}

	static bool is_blacklisted(const std::string & host)
	{
		init();
		return ((DB_blacklist *)DB_Blacklist)->is_blacklisted_(host);
	}

	//returns the time when the blacklist last had a host added
	static time_t last_added()
	{
		init();
		return ((DB_blacklist *)DB_Blacklist)->last_added_();
	}

private:
	//only DB_blacklist can initialize itself
	DB_blacklist();

	//init() must be called at the top of every public function
	static void init()
	{
		//double-checked lock eliminates mutex overhead
		if(DB_Blacklist == NULL){ //non-threadsafe comparison (the hint)
			boost::mutex::scoped_lock lock(init_mutex);
			if(DB_Blacklist == NULL){ //threadsafe comparison
				DB_Blacklist = new volatile DB_blacklist();
			}
		}
	}

	static int is_blacklisted_call_back(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		*((bool *)object_ptr) = true;
		return 0;
	}

	//the one possible instance of DB_blacklist
	static volatile DB_blacklist * DB_Blacklist;
	static boost::mutex init_mutex; //mutex for init()

	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;     //mutex for functions called by public static functions
	time_t last_time_added; //when last host added to blacklist

	/*
	All these functions correspond to public static functions.
	*/
	void add_(const std::string & host);
	bool is_blacklisted_(const std::string & host);
	time_t last_added_();
};
#endif
