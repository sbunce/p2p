#ifndef H_DB_BLACKLIST
#define H_DB_BLACKLIST

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

//std
#include <iostream>
#include <sstream>
#include <sstream>
#include <string>

class DB_blacklist
{
public:
	/*
	Add a host to the blacklist.

	Everytime a IP is added the modified_count is incremented. This is used by
	modified() to communicate back to objects if the blacklist has been added to.
	*/
	static void add(const std::string & IP)
	{
		init();
		++blacklist_state;
		((DB_blacklist *)DB_Blacklist)->add_worker(IP);
	}

	//returns true if the IP is blacklisted
	static bool is_blacklisted(const std::string & IP)
	{
		init();
		return ((DB_blacklist *)DB_Blacklist)->is_blacklisted_worker(IP);
	}

	/*
	This function is used to communicate back to different objects if one of more
	IPs have been added to the blacklist.

	The way this function works is an integer in the client object is initialized
	to zero and passed to this function. The first time this function is used it
	will always return true.

	All other times if blacklist_state_in is different than modified_count true
	will be returned and blacklist_state_in will be set equal to blacklist_state.
	*/
	static bool modified(int & blacklist_state_in)
	{
		if(blacklist_state_in == blacklist_state){
			//blacklist has not been updated
			return false;
		}else{
			//blacklist updated
			blacklist_state_in = blacklist_state;
			return true;
		}
	}

private:
	//only DB_blacklist can initialize itself
	DB_blacklist();

	//init() must be called at the top of every public function
	static void init()
	{
		//double checked lock to avoid locking overhead
		if(DB_Blacklist == NULL){ //unsafe comparison, the hint
			boost::mutex::scoped_lock lock(init_mutex);
			if(DB_Blacklist == NULL){ //threadsafe comparison
				DB_Blacklist = new DB_blacklist();
			}
		}
	}

	static int is_blacklisted_call_back(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		*((bool *)object_ptr) = true;
		return 0;
	}

	//the one possible instance of DB_blacklist
	static DB_blacklist * DB_Blacklist;
	static boost::mutex init_mutex; //mutex for init()

	/*
	This count starts at 1 and is incremented every time a server is added to the
	blacklist.
	*/
	static volatile int blacklist_state;

	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;     //mutex for functions called by public static functions

	/*
	All these functions correspond to public static functions.
	*/
	void add_worker(const std::string & IP);
	bool is_blacklisted_worker(const std::string & IP);
};
#endif
