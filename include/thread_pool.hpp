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

	//clears all pending jobs
	void clear()
	{
		boost::mutex::scoped_lock lock(job_queue_mutex);
		job_queue.clear();
	}

	//add call back job to thread pool
	void queue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(job_queue_mutex);
		job_queue.push_back(func);
		job_queue_cond.notify_one();
	}

	//interrupts threads, block until job_queue empties and threads terminate
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

	/*
	job_queue_mutex:
		Locks access to everything in this section.
	job_queue_cond:
		Used by queue() to signal dispatcher() thread that a job needs to be done.
	job_queue:
		Functions to call back to.
	stop_threads:
		Triggers termination 
	*/
	boost::mutex job_queue_mutex;
	boost::condition_variable_any job_queue_cond;
	std::deque<boost::function<void ()> > job_queue;
	bool stop_threads;

	//threads wait here for jobs
	void dispatcher()
	{
		boost::function<void ()> job;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_queue_mutex);
			while(job_queue.empty()){
				if(stop_threads){
					return;
				}
				job_queue_cond.wait(job_queue_mutex);
			}
			job = job_queue.front();
			job_queue.pop_front();
			}//end lock scope
			job();
		}
	}
};
#endif
