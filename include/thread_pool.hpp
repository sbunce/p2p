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
	//accuracy of timer, may be up to this many ms off
	static const unsigned timer_accuracy_ms = 10;

public:
	thread_pool(const unsigned threads = boost::thread::hardware_concurrency()):
		stopped(false),
		timeout_ms(0),
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
		timeout_thread.interrupt();
		timeout_thread.join();
		workers.interrupt_all();
		workers.join_all();
	}

	//clear jobs
	void clear()
	{
		boost::mutex::scoped_lock lock(mutex);
		job_cnt -= job_queue.size();
		job_queue.clear();
		job_cnt -= timeout_jobs.size();
		timeout_jobs.clear();
		join_cond.notify_all();
	}

	//enqueue job
	bool enqueue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(stopped){
			return false;
		}else{
			job_queue.push_back(func);
			++job_cnt;
			job_cond.notify_one();
			return true;
		}
	}

	//enqueue job after specified delay (ms)
	bool enqueue(const boost::function<void ()> & func, const unsigned delay_ms)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(stopped){
			return false;
		}else{
			if(timeout_thread.get_id() == boost::thread::id()){
				//lazy start timeout_thread
				timeout_thread = boost::thread(boost::bind(&thread_pool::timeout_dispatcher, this));
			}
			if(delay_ms == 0){
				job_queue.push_back(func);
				job_cond.notify_one();
			}else{
				timeout_jobs.insert(std::make_pair(timeout_ms + delay_ms, func));
			}
			++job_cnt;
			return true;
		}
	}

	//block until all jobs done
	void join()
	{
		boost::mutex::scoped_lock lock(mutex);
		while(job_cnt > 0){
			join_cond.wait(mutex);
		}
	}

	//enable enqueue
	void start()
	{
		boost::mutex::scoped_lock lock(mutex);
		stopped = false;
	}

	//disable enqueue
	void stop()
	{
		boost::mutex::scoped_lock lock(mutex);
		stopped = true;
	}

private:
	boost::mutex mutex;                      //locks everyting (object is a monitor)
	boost::thread_group workers;             //dispatcher() threads
	boost::thread timeout_thread;            //timout_dispatcher() thread
	boost::condition_variable_any job_cond;  //notified when job added
	boost::condition_variable_any join_cond; //notified when job added or job finished
	bool stopped;                            //when true no jobs can be added
	boost::uint64_t timeout_ms;              //incremented by timeout_thread

	//new jobs pushed on back, jobs on front processed
	std::list<boost::function<void ()> > job_queue;
	//jobs enqueued after timeout, when timeout_ms >= key
	std::multimap<boost::uint64_t, boost::function<void ()> > timeout_jobs;
	//incremented when job added, decremented when job finished
	unsigned job_cnt;

	//consumes enqueued jobs, does call backs
	void dispatcher()
	{
		while(true){
			boost::function<void ()> tmp;
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			while(job_queue.empty()){
				job_cond.wait(mutex);
			}
			tmp = job_queue.front();
			job_queue.pop_front();
			}//END lock scope
			tmp();
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			--job_cnt;
			}//END lock scope
			join_cond.notify_all();
		}
	}

	//enqueues jobs after a timeout (ms)
	void timeout_dispatcher()
	{
		while(true){
			boost::this_thread::sleep(boost::posix_time::milliseconds(timer_accuracy_ms));
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			timeout_ms += timer_accuracy_ms;
			while(!timeout_jobs.empty() && timeout_ms >= timeout_jobs.begin()->first){
				job_queue.push_back(timeout_jobs.begin()->second);
				job_cond.notify_one();
				timeout_jobs.erase(timeout_jobs.begin());
			}
			}//END lock scope
		}
	}
};
#endif
