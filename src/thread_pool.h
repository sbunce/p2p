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
	join      - blocks until job queue empties out and all threads terminate
	queue     - queue a job, example:
	            member function:                TP.queue(boost::bind(&A::test, &a));
			      member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
	            function:                       TP.queue(&test);
	            function with parameter:        TP.queue(boost::bind(test, "test"));
	terminate - empties work_queue and does a join, this should always be called
	            in destructor of class that has-a thread_pool
	*/
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
	void process();
};
#endif
