#ifndef H_SHARE_SCAN_1_HASH
#define H_SHARE_SCAN_1_HASH

//custom
#include "hash_tree.hpp"
#include "share.hpp"
#include "share_scan_job.hpp"
#include "share_scan_0_scan.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

class share_scan_1_hash
{
public:
	share_scan_1_hash(share & Share_in);
	~share_scan_1_hash();

	/*
	job:
		Returns shared_ptr for job, or empty shared_ptr if no jobs.
	*/
	void block_until_resumed();
	boost::shared_ptr<share_scan_job> job();

private:
	boost::thread_group Workers;

	/*
	job_queue:
		Holds jobs to be returned by job(). Jobs put in this container need to
		have an entry written in the database for the file.
	job_mutex:
		Locks all access to job_queue.
	job_queue_max_cond:
		Used to block worker threads when job limit reached.
	*/
	std::deque<boost::shared_ptr<share_scan_job> > job_queue;
	boost::mutex job_queue_mutex;
	boost::condition_variable_any job_queue_max_cond;

	//contains files in share
	share & Share;

	//previous stage in pipeline
	share_scan_0_scan Share_Scan_0_Scan;

	/*
	block_on_max_jobs:
		Blocks the Workers when the job buffer is full.
	main_loop:
		All the workers run in this function.
	*/
	void block_on_max_jobs();
	void main_loop();
};
#endif
