#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <singleton.hpp>

//standard
#include <list>
#include <map>
#include <set>

class thread_pool : private boost::noncopyable
{
public:
	//0 max_buf is unlimited job queue size
	thread_pool(
		const unsigned threads = boost::thread::hardware_concurrency(),
		const unsigned max_buf_in = 0
	):
		stopped(false),
		max_buf(max_buf_in),
		job_cnt(0)
	{
		for(unsigned x=0; x<threads; ++x){
			workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	~thread_pool()
	{
		stop();
		join();
		workers.interrupt_all();
		workers.join_all();
	}

	//clear jobs
	void clear()
	{
		boost::mutex::scoped_lock lock(mutex);
		job_cnt -= job_queue.size();
		job_queue.clear();
		empty_cond.notify_all();
	}

	//enqueue job, blocks if max_buf reached
	bool enqueue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(stopped){
			return false;
		}else{
			if(max_buf != 0){
				while(job_queue.size() >= max_buf){
					consumer_cond.wait(mutex);
				}
			}
			job_queue.push_back(func);
			++job_cnt;
			producer_cond.notify_one();
		}
	}

	//block until all jobs done
	void join()
	{
		boost::mutex::scoped_lock lock(mutex);
		while(job_cnt > 0){
			empty_cond.wait(mutex);
		}
	}

	//stops new jobs from being enqueued
	void stop()
	{
		boost::mutex::scoped_lock lock(mutex);
		stopped = true;
	}

	//allows new jobs to be enqueued
	void start()
	{
		boost::mutex::scoped_lock lock(mutex);
		stopped = false;
	}

private:
	boost::mutex mutex;                             //locks everything
	bool stopped;                                   //if true new jobs not allowed to be enqueued
	boost::thread_group workers;                    //all dispatcher threads
	boost::condition_variable_any consumer_cond;    //notify_one when job taken from job_queue
	boost::condition_variable_any producer_cond;    //notify_one when job added to job_queue
	boost::condition_variable_any empty_cond;       //notify_all when no jobs scheduled or running
	std::list<boost::function<void ()> > job_queue; //enqueue on back, pop from front
	unsigned max_buf;                               //max_allowed job_queue size
	unsigned job_cnt;                               //cnt of queue'd + running jobs

	//threads which consume jobs reside here
	void dispatcher()
	{
		while(true){
			boost::function<void ()> tmp;
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			while(job_queue.empty()){
				producer_cond.wait(mutex);
			}
			tmp = job_queue.front();
			job_queue.pop_front();
			}//END lock scope
			tmp();
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			--job_cnt;
			if(job_cnt == 0){
				empty_cond.notify_all();
			}
			}//END lock scope
			consumer_cond.notify_one();
		}
	}
};
#endif
