#ifndef H_SHARE_PIPELINE_1_HASH
#define H_SHARE_PIPELINE_1_HASH

//custom
#include "share_pipeline_job.hpp"
#include "share_pipeline_0_scan.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

class share_pipeline_1_hash
{
public:
	share_pipeline_1_hash();

	/*
	get_job:
		Returns true and sets info to a job if there is a job to be done. Returns
		false and leaves info unset if there is no job. This function does not
		block.
	share_size:
		Returns the size of the share. (bytes)
	stop:
		Stops all threads in Workers group that are hashing files.
	*/
	bool get_job(share_pipeline_job & info);
	boost::uint64_t share_size();
	void stop();

private:
	boost::thread_group Workers;

	//jobs for share_pipeline_1_scan process
	std::deque<share_pipeline_job> job;
	boost::mutex job_mutex;

	//used to block Worker threads when job limit reached
	boost::condition_variable_any job_max_cond;

	/*
	block_on_max_jobs:
		Blocks the Workers when the job buffer is full.
	main_loop:
		All the workers run in this function.
	*/
	void block_on_max_jobs();
	void main_loop();

	//the stage before this stage
	share_pipeline_0_scan Share_Pipeline_0_Scan;
};
#endif
