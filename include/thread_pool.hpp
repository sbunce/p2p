#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

//standard
#include <queue>

/* Function Naming
Some functions have the following words present in their names. They each have
specific meaning.
clear:
	Remove all jobs from thread_pool.
	Note: This does not cancel currently running jobs.
stop:
	Don't allow new jobs to be enqueued. The enqueue function will return false
	and fail to add a job.
join:
	Block until all queued jobs and all running jobs complete.

Combination functions are offered because the different operations need to
happen inside one lock.
*/
class thread_pool : private boost::noncopyable
{
public:
	//start thread_pool
	thread_pool(const unsigned threads = boost::thread::hardware_concurrency()):
		running_jobs(0),
		stopped(false),
		dtor_stopped(false)
	{
		for(unsigned x=0; x<threads; ++x){
			workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	~thread_pool()
	{
		//don't allow any more jobs to be enqueued
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		dtor_stopped = true;
		}//END lock scope

		//wait for existing jobs to finish
		join();

		//stop threads
		workers.interrupt_all();
		workers.join_all();
	}

	//enqueue job, returns false if job not enqueued
	bool enqueue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		if(stopped || dtor_stopped){
			return false;
		}else{
			job_queue.push_back(func);
			job_cond.notify_one();
			return true;
		}
	}

	//see documentation at top of this file
	void join()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		while(!job_queue.empty() || running_jobs){
			join_cond.wait(job_mutex);
		}
	}

	//start thread_pool, allow jobs to be enqueued
	void start()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		stopped = false;
	}

	//see documentation at top of this file
	void stop_join()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		stopped = true;
		while(!job_queue.empty() || running_jobs){
			join_cond.wait(job_mutex);
		}
	}

	//see documentation at top of this file
	void stop_clear_join()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		stopped = true;
		job_queue.clear();
		while(!job_queue.empty() || running_jobs){
			join_cond.wait(job_mutex);
		}
	}

private:
	boost::mutex job_mutex;                          //locks everyting in this section
	boost::thread_group workers;                     //worker threads
	boost::condition_variable_any job_cond;          //cond notified when job added
	std::deque<boost::function<void ()> > job_queue; //jobs to run
	unsigned running_jobs;                           //number of jobs in progress
	boost::condition_variable_any join_cond;         //cond used for join
	bool stopped;                                    //when true no jobs can be added
	bool dtor_stopped;                               //dtor called, thread_pool can't be started again
	
	void dispatcher()
	{
		boost::function<void ()> job;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			while(job_queue.empty()){
				job_cond.wait(job_mutex);
			}
			job = job_queue.front();
			job_queue.pop_front();
			++running_jobs;
			}//end lock scope
			job();
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			--running_jobs;
			}//end lock scope
			join_cond.notify_all();
		}
	}
};
#endif
