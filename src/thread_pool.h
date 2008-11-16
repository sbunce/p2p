#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//custom
#include "atomic_int.h"
#include "global.h"

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

//std
#include <iostream>
#include <queue>

class thread_pool
{
public:
	thread_pool(const int & thread_limit_in);
	~thread_pool();

	/*
	join  - blocks until job all jobs finished and all threads terminate
	max   - set maximum threads allowed in Pool
	queue - queue a job, example:
	        member function:                TP.queue(boost::bind(&A::test, &a));
	        member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
	        function:                       TP.queue(&test);
	        function with parameter:        TP.queue(boost::bind(test, "test"));
	*/
	void join();
	void max(int threads);
	void queue(const boost::function0<void> & func);

private:
	atomic_int<int> thread_limit; //maximum number of threads allowed to run
	boost::thread_group Pool;     //all threads currently in the thread pool

	/*
	Work queue which contains an object pointer and a member function to run in
	that object
	*/
	std::queue<boost::function0<void> > work_queue;
	boost::mutex work_queue_mutex;

	/*
	process - process job queue
	*/
	void process(boost::thread * T);
};
#endif
