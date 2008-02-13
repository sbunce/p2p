/*
	Copyright (c) 2008 Seth Bunce (seth@seth.dnsdojo.net)

	thread_pool.h is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	thread_pool.h is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//std
#include <queue>

#include "atomic.h"

template<class T> class thread_pool
{
public:
	thread_pool();          //does not star threads, use init()
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

	Example calling from inside an object:
	void (T::*memfun_ptr)() = &T::call_back;
	Thread_Pool->queue_job(this, memfun_ptr);
	*/
	void queue_job(T * Obj, void (T::*memfun_ptr)());

private:
	atomic<bool> stop_threads;      //true when threads are being stopped
	atomic<int> threads; //how many threads are currently running

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

template<class T> thread_pool<T>::thread_pool()
{
	stop_threads = false;
	threads = 0;
}

template<class T> thread_pool<T>::thread_pool(int start)
{
	thread_pool();
	for(int x=0; x<start; ++x){
		boost::thread Thread(boost::bind(&thread_pool::pool, this));
	}
}

template<class T> void thread_pool<T>::init(int start)
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

	void (T::*memfun_ptr)(); //member function pointer
	T * Obj;                 //object pointer

	while(true){

		usleep(100000);

		if(stop_threads){
			break;
		}

		{//begin lock scope
		boost::mutex::scoped_lock lock(Mutex);

		if(!work_queue.empty()){
			//this needs to be outside the mutex
			Obj = work_queue.front().first;
			memfun_ptr = work_queue.front().second;
			work_queue.pop();
		}
		else{
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

	//spin-lock idea, wait for threads to terminate
	while(threads){
		usleep(100);
	}
}
#endif
