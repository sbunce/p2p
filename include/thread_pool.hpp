#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

//standard
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

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

	/*
	queue:
		Add call back job for the thread pool. Example:
		member function:                TP.queue(boost::bind(&A::test, &a));
		member function with parameter: TP.queue(boost::bind(&A::test, &a, "test"));
		function:                       TP.queue(&test);
		function with parameter:        TP.queue(boost::bind(test, "test"));
	*/
	void queue(const boost::function0<void> & CB)
	{
		boost::mutex::scoped_lock lock(call_back_mutex);
		call_back.push_back(CB);
		call_back_cond.notify_one();
	}

	//causes threads to terminate after job queue empties out
	void interrupt()
	{
		{//begin lock scope
		boost::mutex::scoped_lock lock(call_back_mutex);
		stop_threads = true;
		}//end lock scope
		call_back_cond.notify_all();
	}

	//blocks until threads have terminated
	void join()
	{
		Workers.join_all();
	}

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

	//threads wait here for jobs
	void dispatcher()
	{
		boost::function0<void> CB;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(call_back_mutex);
			while(call_back.empty()){
				if(stop_threads){
					//only stop thread when there are no more jobs
					return;
				}
				call_back_cond.wait(call_back_mutex);
			}
			CB = call_back.front();
			call_back.pop_front();
			}//end lock scope
			CB();
		}
	}
};
#endif
