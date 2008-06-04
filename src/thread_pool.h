#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//std
#include <queue>

//custom
#include "global.h"

template<class T> class thread_pool
{
public:
	thread_pool(int start); //starts threads

	/*
	init - starts threads
	stop - stops all threads in pool()
	*/
	void init(int start);
	void stop();

	/*
	Call this function to queue a job. It will be queued and served when there
	is an available thread. T is the class type and call_back is the function
	you want run in a thread. Note that the function signature must match this
	(void return type, no parameters).

	void (T::*memfun_ptr)() = &T::call_back;
	Thread_Pool->queue_job(this, memfun_ptr);
	*/
	void queue_job(T * Obj, void (T::*memfun_ptr)());

private:
	volatile bool stop_threads; //true when threads are being stopped
	volatile int threads;       //how many threads are currently running

	/*
	Work queue which contains an object pointer and a member function to run in
	that object
	*/
	std::queue<std::pair<T*, void (T::*)()> > work_queue;
	boost::mutex Mutex;

	/*
	pool - where the threads chillax and wait for work
	*/
	void pool();
};

template<class T> thread_pool<T>::thread_pool(int start)
{
	for(int x=0; x<start; ++x){
		boost::thread Thread(boost::bind(&thread_pool::pool, this));
	}
}

template<class T> void thread_pool<T>::queue_job(T * Obj, void (T::*memfun_ptr)())
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	work_queue.push(std::make_pair(Obj, memfun_ptr));
	}//end lock scope
}

template<class T> void thread_pool<T>::pool()
{
	++threads;

	void (T::*memfun_ptr)();
	T * Obj;
	while(true){
		usleep(1);
		if(stop_threads){
			break;
		}
		{//begin lock scope
		boost::mutex::scoped_lock lock(Mutex);
		if(!work_queue.empty()){
			Obj = work_queue.front().first;
			memfun_ptr = work_queue.front().second;
			work_queue.pop();
		}else{
			continue;
		}
		}//end lock scope

		(*Obj.*memfun_ptr)();
	}

	--threads;
}

template<class T> void thread_pool<T>::stop()
{
	stop_threads = true;
	while(threads){
		usleep(1);
	}
}
#endif
