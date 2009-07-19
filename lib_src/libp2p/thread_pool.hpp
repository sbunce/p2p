//SINGLETON, THREADSAFE, THREAD SPAWNING
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

//standard
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

class thread_pool : public singleton_base<thread_pool>
{
	friend class singleton_base<thread_pool>;
public:
	thread_pool():
		stop_threads(false)
	{}

	void start()
	{
		for(int x=0; x<settings::THREAD_POOL_SIZE; ++x){
			Workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	void stop()
	{
		{//begin lock scope
		boost::mutex::scoped_lock lock(call_back_mutex);
		stop_threads = true;
		}//end lock scope
		call_back_cond.notify_all();
		Workers.join_all();
	}

	/*
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

	void dispatcher()
	{
		boost::function0<void> CB;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(call_back_mutex);
			while(call_back.empty()){
				//only stop thread when there are no more jobs
				if(stop_threads){
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
