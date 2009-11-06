#ifndef H_SHARE_SCANNER
#define H_SHARE_SCANNER

//custom
#include "file_info.hpp"
#include "hash_tree.hpp"
#include "share.hpp"

//include
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <logger.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <cassert>
#include <deque>
#include <map>
#include <string>

class share_scanner : private boost::noncopyable
{
public:
	share_scanner();
	~share_scanner();

	/*
	block_until_resumed:
		Blocks until the share has been fully repopulated.
	*/
	void block_until_resumed();

private:
	boost::thread_group Workers;

	/*
	Stores the paths of files that are currently hashing associated with a
	time. If the time != 0 then the element won't be unmemoized until >= time.
	*/
	boost::mutex memoize_mutex;
	std::map<std::string, std::time_t> memoize;

	/*
	job_queue_mutex:
		Locks all access to job_queue.
	joe_queue_cond:
		Used to notify a thread blocked at get_job() that a job can be returned.
	job_queue_max_cond:
		When the job_queue reaches the max allowed size the scan_thread is blocked
		on job_queue_max_cond. It is notified in get_job() when a job is returned.
	job_queue:
		Holds file_info for files that need to be hashed.
	*/
	boost::mutex job_queue_mutex;
	boost::condition_variable_any job_queue_cond;
	boost::condition_variable_any job_queue_max_cond;
	std::deque<file_info> job_queue;

	/*
	Used by block_until_resumed() to block until share is populated with
	information in database.
	Note: All access to resumed is locked with resumed_mutex.
	*/
	boost::mutex resumed_mutex;
	bool resumed;
	boost::condition_variable_any resumed_cond;

	/*
	hash_loop:
		Threads wait in this function for files to hash.
	scan_loop:
		One thread scans the shared directories.
	*/
	void hash_loop();
	void scan_loop();
};
#endif
