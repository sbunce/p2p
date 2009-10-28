#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

//standard
#include <iostream>
#include <limits>
#include <queue>

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
		boost::mutex::scoped_lock lock(job_queue_mutex);
		while(!job_queue.empty()){
			job_queue.pop();
		}
	}

	/*
	queue:
		Add call back job for the thread pool. An optional priority may be specified.
		If none is specified the lowest priority is assigned.
	Example:
		member function:                TP.queue(boost::bind(&A::test, &a));
		member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
		function:                       TP.queue(&test);
		function with parameter:        TP.queue(boost::bind(test, "test"));
	*/
	void queue(const boost::function<void ()> & func,
		const int priority = std::numeric_limits<int>::min())
	{
		boost::mutex::scoped_lock lock(job_queue_mutex);
		job_queue.push(job(func, priority));
		job_queue_cond.notify_one();
	}

	/*
	Triggers all threads to terminate when the job queue is empty. Blocks until
	all threads have terminated.
	Note: Call clear() if the jobs aren't important and can be discarded.
	*/
	void interrupt_join()
	{
		{//begin lock scope
		boost::mutex::scoped_lock lock(job_queue_mutex);
		stop_threads = true;
		}//end lock scope
		job_queue_cond.notify_all();
		Workers.join_all();
	}

private:
	boost::thread_group Workers;

	class job
	{
	public:
		job(){}

		job(const boost::function<void ()> & func_in, const int priority_in):
			func(func_in), priority(priority_in)
		{}

		job(const job & Job):
			func(Job.func), priority(Job.priority)
		{}

		void run()
		{
			func();
		}

		job & operator = (const job & rval)
		{
			func = rval.func;
			priority = rval.priority;
			return *this;
		}

		bool operator < (const job & rval) const
		{
			return priority < rval.priority;
		}

	private:
		boost::function<void ()> func;
		int priority;
	};

	//used by queue_job to transfer jobs to threads in conumer
	boost::mutex job_queue_mutex;
	boost::condition_variable_any job_queue_cond;
	std::priority_queue<job> job_queue;

	/*
	Triggers worker threads to exit after finishing jobs.
	Note: Must be locked with job_mutex.
	*/
	bool stop_threads;

	//threads wait here for jobs
	void dispatcher()
	{
		job Job;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_queue_mutex);
			while(job_queue.empty()){
				if(stop_threads){
					return;
				}
				job_queue_cond.wait(job_queue_mutex);
			}
			Job = job_queue.top();
			job_queue.pop();
			}//end lock scope
			Job.run();
		}
	}
};
#endif
