#ifndef H_SHARE_SCAN_0_SCAN
#define H_SHARE_SCAN_0_SCAN

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "shared_files.hpp"
#include "share_scan_job.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <logger.hpp>

//standard
#include <cassert>
#include <deque>
#include <map>
#include <string>

class share_scan_0_scan : private boost::noncopyable
{
public:
	share_scan_0_scan(shared_files & Shared_Files_in);
	~share_scan_0_scan();

	/*
	job:
		Blocks until a job is ready.
	*/
	boost::shared_ptr<share_scan_job> job();

private:
	boost::thread scan_thread;

	/*
	job_queue:
		Holds jobs to be returned by job().
	job_queue_mutex:
		Locks all access to job_queue.
	joe_queue_cond:
		Used to notify a thread blocked at get_job() that a job can be returned.
	job_queue_max_cond:
		When the job_queue reaches the max allowed size the scan_thread is blocked
		on job_queue_max_cond. It is notified in get_job() when a job is returned.
	*/
	std::deque<boost::shared_ptr<share_scan_job> > job_queue;
	boost::mutex job_queue_mutex;
	boost::condition_variable_any job_queue_cond;
	boost::condition_variable_any job_queue_max_cond;

	//contains files in share
	shared_files & Shared_Files;

	/*
	block_on_max_jobs:
		Blocks the scan thread when the job buffer is full.
	main_loop:
		Function scan_thread operates in.
	remove_missing:
		Removes missing files from File container.
	*/
	void block_on_max_jobs();
	void main_loop();
};
#endif
