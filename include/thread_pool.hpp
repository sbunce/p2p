//THREADSAFE
#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//boost
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//include
#include "logger.hpp"

//std
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

class thread_pool : private boost::noncopyable
{
public:
	thread_pool(const unsigned & threads):
		active_jobs(0)
	{
		for(int x=0; x<threads; ++x){
			Pool.create_thread(boost::bind(&thread_pool::consumer, this));
		}
	}
	~thread_pool()
	{
		Pool.interrupt_all();
		queue_cond.notify_all();
		try{
			Pool.join_all();
		}catch(boost::thread_interrupted ex){
			LOGGER;
		}
	}

	/*
	member function:                TP.queue(boost::bind(&A::test, &a));
	member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
	function:                       TP.queue(&test);
	function with parameter:        TP.queue(boost::bind(test, "test"));
	*/
	void queue_job(const boost::function0<void> & job)
	{
		boost::mutex::scoped_lock lock(queue_mutex);
		queue.push_back(job);
		queue_cond.notify_one();
	}

	//wait for threads to finish all jobs
	void join()
	{
		boost::mutex::scoped_lock lock(queue_mutex);
		while(active_jobs != 0 || !queue.empty()){
			queue_empty_cond.wait(queue_mutex);
		}
	}

private:
	boost::thread_group Pool;

	//used by queue_job to transfer jobs to threads in conumer
	std::deque<boost::function0<void> > queue;
	boost::condition_variable_any queue_cond;
	boost::mutex queue_mutex;

	//number of consumer threads processing jobs, must be locked with queue_mutex
	unsigned active_jobs;

	//notified when
	boost::condition_variable_any queue_empty_cond;

	void consumer()
	{
		boost::function0<void> job;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(queue_mutex);
			while(queue.empty()){
				queue_cond.wait(queue_mutex);
			}
			++active_jobs;
			boost::this_thread::interruption_point();
			job = queue.front();
			queue.pop_front();
			}//end lock scope
			job();

			{//begin lock scope
			boost::mutex::scoped_lock lock(queue_mutex);
			--active_jobs;
			queue_empty_cond.notify_all();
			}//end lock scope
		}
	}
};
#endif
