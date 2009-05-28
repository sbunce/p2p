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
	share_size:
		Returns size of shared files. (bytes)
	stop:
		Stops all worker threads in preparation for program shut down.
	*/
	boost::uint64_t share_size();
	void stop();

private:
	share_index();

	//threads to generate hash trees
	boost::thread_group Workers;

	/*
	This thread is used to combine updates to the database in a transaction.
	Without this database performance becomes abysmal.
	*/
	boost::thread write_combine_thread;


//DEBUG, need to have a max size on this at which the workers get blocked
	/*
	Jobs completed by the workers.
	Note: All access to this container must be locked.
	*/
	std::vector<share_scan::job_info> write_combine_job;
	boost::mutex write_combine_job_mutex;

	/*
	remove_temporary_files:
		Remove all temporary files used for generating hash trees by the different
		worker threads.
	worker_pool:
		Where workers wait for files from share_scan to hash.
	write_combine:
		Combines the adding and removing of records to the share and hash table in
		to transactions.
	*/
	void remove_temporary_files();
	void worker_pool();
	void write_combine();

	share_scan Share_Scan;
};
#endif

