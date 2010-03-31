#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

//standard
#include <list>
#include <set>

class thread_pool
{
public:
	thread_pool(const unsigned threads = boost::thread::hardware_concurrency())
	{
		for(unsigned x=0; x<threads; ++x){
			workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	~thread_pool()
	{
		interrupt_join();
	}

	//remove all jobs
	void clear()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job_queue.clear();
	}

	//interrupt all threads and block until they terminate
	void interrupt_join()
	{
		workers.interrupt_all();
		workers.join_all();
	}

	//queue a job
	void queue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job_queue.push_back(func);
		job_cond.notify_one();
	}

private:
	/*
	workers:
		All threads in the thread_pool.
	job_mutex:
		Locks access to all in this section.
	job_queue:
		New jobs added to the back, jobs to run are taken from front.
	*/
	boost::thread_group workers;
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<boost::function<void ()> > job_queue;
	
	void dispatcher()
	{
		boost::function<void ()> job;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			while(job_queue.empty()){
				job_cond.wait(job_mutex);
			}
			job = job_queue.front();
			job_queue.pop_front();
			}//end lock scope
			job();
		}
	}
};
#endif
