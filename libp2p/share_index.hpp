//SINGLETON, THREADSAFE, THREAD SPAWNING
#ifndef H_SHARE_INDEX
#define H_SHARE_INDEX

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/utility.hpp>

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "share_scan.hpp"

//include
#include <atomic_int.hpp>
#include <portable_sleep.hpp>
#include <singleton.hpp>
#include <thread_pool.hpp>

//std
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

class share_index : public singleton_base<share_index>
{
	friend class singleton_base<share_index>;
public:
	/*
	is_indexing:
		Returns true if the server_index is hashing files.
	stop:
		Stops all worker threads in preparation for program shut down.
	*/
	bool is_indexing();
	void stop();

private:
	share_index();

	//threads to generate hash trees
	boost::thread_group Workers;

	//how many workers are currently hashing
	atomic_int<int> workers_hashing;

	/*
	worker_pool:
		Where workers wait for files from share_scan to hash.
	*/
	void remove_temporary_files();
	void worker_pool();

	share_scan Share_Scan;
	database::connection DB;
};
#endif

