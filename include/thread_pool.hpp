#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

//standard
#include <queue>

class thread_pool : private boost::noncopyable
{
public:
	thread_pool(const unsigned threads = boost::thread::hardware_concurrency()):
		running_jobs(0),
		is_stopped(false)
	{
		for(unsigned x=0; x<threads; ++x){
			workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	~thread_pool()
	{
		workers.interrupt_all();
		workers.join_all();
	}

	//place function in job queue
	void enqueue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		if(!is_stopped){
			job_queue.push_back(func);
			job_cond.notify_one();
		}
	}

	//block until all jobs completed
	void join()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		while(!job_queue.empty() || running_jobs){
			join_cond.wait(job_mutex);
		}
	}

	//stop new jobs from being added and block until existing jobs complete
	void stop()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		//insure no new jobs get added and wait for existing jobs to finish
		is_stopped = true;
		while(!job_queue.empty() || running_jobs){
			join_cond.wait(job_mutex);
		}
	}

private:
	boost::thread_group workers;                     //all threads in thread_pool
	boost::mutex job_mutex;                          //locks everyting in this section
	boost::condition_variable_any job_cond;          //cond notified when job added
	std::deque<boost::function<void ()> > job_queue; //jobs to run
	unsigned running_jobs;                           //number of jobs in progress
	boost::condition_variable_any join_cond;         //cond used for join
	bool is_stopped;                                 //when true no jobs can be added
	
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
			++running_jobs;
			}//end lock scope
			job();
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			--running_jobs;
			}//end lock scope
			join_cond.notify_all();
		}
	}
};
#endif
