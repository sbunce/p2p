#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

//std
#include <string>

//custom
#include "DB_access.h"
#include "global.h"

class server_index
{
public:
	server_index();
	/*
	is_indexing - true if indexing(scanning for new files and hashing)
	start       - starts the index share thread
	stop        - stops the index share thread (blocks until thread stopped, up to 1 second)
	*/
	bool is_indexing();
	void start();
	void stop();

private:
	volatile bool stop_thread; //if true this will trigger thread termination
	volatile int threads;      //how many threads are currently running
	volatile bool indexing;    //true if server_index is currently indexing files

	/*
	index_share         - removes files listed in index that don't exist in share
	index_share_recurse - recursive function to locate all files in share(calls add_entry)
	*/
	void index_share();
	void index_share_recurse(std::string directory_name);

	DB_access DB_Access;
};
#endif

