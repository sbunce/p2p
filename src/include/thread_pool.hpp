/*
This stops compilation when boost is too old. The MIN_MAJOR_VERSION and
MIN_MINOR_VERSION need to be set to the minimum allowed version of boost.
example: 1.35 would have major 1 and minor 35
*/
#include <boost/version.hpp>
#define MIN_MAJOR_VERSION 1
#define MIN_MINOR_VERSION 35

#define CURRENT_MAJOR_VERSION BOOST_VERSION / 100000
#define CURRENT_MINOR_VERSION BOOST_VERSION / 100 % 1000

#if CURRENT_MAJOR_VERSION < MIN_MAJOR_VERSION
	#error boost version too old
#elif CURRENT_MAJOR_VERSION == MIN_MAJOR_VERSION
	#if CURRENT_MINOR_VERSION < MIN_MINOR_VERSION
		#error boost version too old
	#endif
#endif

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
	thread_pool(const unsigned & threads)
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
		queue_mutex.lock();
		queue.push_back(job);
		queue_mutex.unlock();
		queue_cond.notify_one();
	}

	//wait for threads to finish all jobs
	void join()
	{
		queue_empty_cond.wait(queue_empty_mutex);
	}

private:
	boost::thread_group Pool;

	//used by queue_job to transfer jobs to threads in conumer
	std::deque<boost::function0<void> > queue;
	boost::condition_variable_any queue_cond;
	boost::mutex queue_mutex;

	//used for consumer() to let join() know when all jobs done
	boost::condition_variable_any queue_empty_cond;
	boost::mutex queue_empty_mutex;

	void consumer()
	{
		boost::function0<void> job;
		try{
			while(true){
				queue_mutex.lock();
				while(queue.empty()){
					queue_cond.wait(queue_mutex);
				}
				boost::this_thread::interruption_point();
				job = queue.front();
				queue.pop_front();
				queue_mutex.unlock();
				job();

				queue_mutex.lock();
				if(queue.empty()){
					queue_empty_cond.notify_all();
				}
				queue_mutex.unlock();
			}
		}catch(const boost::thread_interrupted & ex){
			queue_mutex.unlock();
		}
	}
};
#endif
