/*
SINGLETON, THREADSAFE, THREAD SPAWNING

Note: If this singleton is constructed then the stop function must be manually
called to properly shut down the threads before program termination.
*/
#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//custom
#include "settings.hpp"

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <logger.hpp>
#include <singleton.hpp>

//standard
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

class thread_pool : public singleton_base<thread_pool>
{
	friend class singleton_base<thread_pool>;
public:
	thread_pool();

	/*
	queue:
		Add call back job for the thread pool. Example:
		member function:                TP.queue(boost::bind(&A::test, &a));
		member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
		function:                       TP.queue(&test);
		function with parameter:        TP.queue(boost::bind(test, "test"));
	stop:
		Stops all threads in pool.
	*/
	void queue(const boost::function0<void> & CB);
	void stop();

private:
	boost::thread_group Workers;

	//used by queue_job to transfer jobs to threads in conumer
	std::deque<boost::function0<void> > call_back;
	boost::condition_variable_any call_back_cond;
	boost::mutex call_back_mutex;

	/*
	Triggers worker threads to exit after finishing jobs.
	Note: Must be locked with queue_mutex.
	*/
	bool stop_threads;

	/*
	dispatcher:
		Threads wait here for call backs to do.
	*/
	void dispatcher();
};
#endif
