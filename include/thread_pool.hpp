#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

//standard
#include <iostream>
#include <limits>
#include <deque>

class thread_pool
{
public:
	thread_pool(const int threads):
		stop_threads(false)
	{
		for(int x=0; x<threads; ++x){
			Workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	//erases all pending jobs
	void clear()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.clear();
	}

	/*
	queue:
		Add call back job for the thread pool.
	Example:
		member function:                TP.queue(boost::bind(&A::test, &a));
		member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
		function:                       TP.queue(&test);
		function with parameter:        TP.queue(boost::bind(test, "test"));
	*/
	void queue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_back(func);
		job_cond.notify_one();
	}

	/*
	Triggers all threads to terminate when the job queue is empty. Blocks until
	all threads have terminated.
	Note: Call clear() if the jobs aren't important and can be discarded.
	*/
	void interrupt_join()
	{
		{//begin lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		stop_threads = true;
		}//end lock scope
		job_cond.notify_all();
		Workers.join_all();
	}

private:
	boost::thread_group Workers;

	//used by queue_job to transfer jobs to threads in conumer
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<boost::function<void ()> > job;

	/*
	Triggers worker threads to exit after finishing jobs.
	Note: Must be locked with job_mutex.
	*/
	bool stop_threads;

	//threads wait here for jobs
	void dispatcher()
	{
		boost::function<void ()> func;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			while(job.empty()){
				if(stop_threads){
					return;
				}
				job_cond.wait(job_mutex);
			}
			func = job.front();
			job.pop_front();
			}//end lock scope
			func();
		}
	}
};
#endif
