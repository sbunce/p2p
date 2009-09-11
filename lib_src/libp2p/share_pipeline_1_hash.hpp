#ifndef H_SHARE_PIPELINE_1_HASH
#define H_SHARE_PIPELINE_1_HASH

//custom
#include "share_info.hpp"
#include "share_pipeline_job.hpp"
#include "share_pipeline_0_scan.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

class share_pipeline_1_hash
{
public:
	share_pipeline_1_hash(share_info & Share_Info_in);
	~share_pipeline_1_hash();

	/*
	job:
		Returns shared_ptr for job, or empty shared_ptr if no jobs.
	*/
	boost::shared_ptr<share_pipeline_job> job();

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
	std::deque<boost::shared_ptr<share_pipeline_job> > job_queue;
	boost::mutex job_queue_mutex;
	boost::condition_variable_any job_queue_max_cond;

	//contains files in share
	share_info & Share_Info;

	//previous stage in pipeline
	share_pipeline_0_scan Share_Pipeline_0_Scan;

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
