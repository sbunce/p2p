#ifndef H_THREAD_POOL
#define H_THREAD_POOL

//include
#include <atomic_bool.hpp>
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
		timeout_ms(0)
	{
		for(unsigned x=0; x<threads; ++x){
			workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	~thread_pool()
	{
		join();
		workers.interrupt_all();
		workers.join_all();
		timeout_thread.interrupt();
		timeout_thread.join();
	}

	//clear jobs
	void clear(void * object_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(object_ptr == NULL){
			job_queue.clear();
			timeout_jobs.clear();
			job_objects.clear();
		}else{
			for(std::list<job>::iterator it_cur = job_queue.begin(); it_cur != job_queue.end();){
				if(it_cur->object_ptr == object_ptr){
					std::multiset<void *>::iterator it = job_objects.find(object_ptr);
					assert(it != job_objects.end());
					job_objects.erase(it);
					it_cur = job_queue.erase(it_cur);
				}else{
					++it_cur;
				}
			}
			for(std::multimap<boost::uint64_t, job>::iterator it_cur = timeout_jobs.begin();
				it_cur != timeout_jobs.end();)
			{
				if(it_cur->second.object_ptr == object_ptr){
					std::multiset<void *>::iterator it = job_objects.find(object_ptr);
					assert(it != job_objects.end());
					job_objects.erase(it);
					timeout_jobs.erase(it_cur++);
				}else{
					++it_cur;
				}
			}
		}
		join_cond.notify_all();
	}

	//enqueue job
	bool enqueue(const boost::function<void ()> & func, void * object_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(stopped){
			return false;
		}else{
			job_queue.push_back(job(func, object_ptr));
			job_objects.insert(object_ptr);
			job_cond.notify_one();
			return true;
		}
	}

	//enqueue job after specified delay (ms)
	bool enqueue(const boost::function<void ()> & func, const unsigned delay_ms,
		void * object_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(stopped){
			return false;
		}else{
			if(timeout_thread.get_id() == boost::thread::id()){
				//lazy start timeout_thread
				timeout_thread = boost::thread(boost::bind(&thread_pool::timeout_dispatcher, this));
			}
			timeout_jobs.insert(std::make_pair(timeout_ms + delay_ms, job(func, object_ptr)));
			job_objects.insert(object_ptr);
			return true;
		}
	}

	//block until all jobs done
	void join(void * object_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(object_ptr == NULL){
			//join on all
			while(!job_objects.empty()){
				join_cond.wait(mutex);
			}
		}else{
			//join on jobs from specific object
			while(job_objects.find(object_ptr) != job_objects.end()){
				join_cond.wait(mutex);
			}
		}
	}

	//returns # of workers thread pool manages
	unsigned size()
	{
		return workers.size();
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
	class job
	{
	public:
		job(
			const boost::function<void ()> & func_in = boost::function<void ()>(),
			void * object_ptr_in = NULL
		):
			func(func_in),
			object_ptr(object_ptr_in)
		{}
		boost::function<void ()> func;
		void * object_ptr;
	};

	boost::mutex mutex;                      //locks everyting
	boost::thread_group workers;             //worker threads
	boost::thread timeout_thread;            //timout_dispatcher() thread
	boost::condition_variable_any job_cond;  //cond notified when job added
	boost::condition_variable_any join_cond; //cond used for join
	bool stopped;                            //when true no jobs can be added
	boost::uint64_t timeout_ms;              //incremented by timeout_thread

	//new jobs pushed on back, dispatcher processes jobs on front
	std::list<job> job_queue;
	//jobs enqueued after timeout, when timeout_ms >= key
	std::multimap<boost::uint64_t, job> timeout_jobs;
	//object pointers for pending and running jobs
	std::multiset<void *> job_objects;

	//consumes enqueued jobs, does call backs
	void dispatcher()
	{
		while(true){
			job tmp;
			{//begin lock scope
			boost::mutex::scoped_lock lock(mutex);
			while(job_queue.empty()){
				job_cond.wait(mutex);
			}
			tmp = job_queue.front();
			job_queue.pop_front();
			}//end lock scope
			tmp.func();
			{//begin lock scope
			boost::mutex::scoped_lock lock(mutex);
			std::multiset<void *>::iterator it = job_objects.find(tmp.object_ptr);
			if(it != job_objects.end()){
				job_objects.erase(it);
			}
			}//end lock scope
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

/*
Global thread pool for CPU bound jobs.
Note: Generally thread_pool_CPU would be used instead of this.
*/
class thread_pool_global_CPU : public singleton_base<thread_pool_global_CPU>
{
	friend class singleton_base<thread_pool_global_CPU>;
public:

	thread_pool & get()
	{
		return TP;
	}

private:
	thread_pool TP;
};

/*
Wrapper automatically specifies object_ptr and automatically joins upon
destruction. It also makes start/stop specific to the object and not the whole
global thread pool.
Note: Should be instantiated in very bottom of header so join works correctly.
*/
class thread_pool_CPU : private boost::noncopyable
{
public:
	/*
	Pass in NULL for non-member func use.
	Note: Default NULL parameter not used because it would be error prone. It'd
		be easy to forget to specify object in initializer list if default
		initialization allowed.
	*/
	thread_pool_CPU(void * object_ptr_in):
		object_ptr(object_ptr_in),
		stopped(false)
	{}

	~thread_pool_CPU()
	{
		join();
	}

	void clear()
	{
		thread_pool_global_CPU::singleton().get().clear(object_ptr);
	}

	bool enqueue(const boost::function<void ()> & func)
	{
		if(stopped){
			return false;
		}else{
			return thread_pool_global_CPU::singleton().get().enqueue(func, object_ptr);
		}
	}

	bool enqueue(const boost::function<void ()> & func, const unsigned delay_ms)
	{
		if(stopped){
			return false;
		}else{
			return thread_pool_global_CPU::singleton().get().enqueue(func, delay_ms, object_ptr);
		}
	}

	void join()
	{
		thread_pool_global_CPU::singleton().get().join(object_ptr);
	}

	unsigned size()
	{
		return thread_pool_global_CPU::singleton().get().size();
	}

	void start()
	{
		stopped = false;
	}

	void stop()
	{
		stopped = true;
	}

private:
	void * object_ptr;
	atomic_bool stopped;
};

/*
Global thread pool for IO bound jobs.
Note: Generally thread_pool_IO would be used instead of this.
*/
class thread_pool_global_IO : public singleton_base<thread_pool_global_IO>
{
	friend class singleton_base<thread_pool_global_IO>;
public:
	thread_pool_global_IO():
		TP(4)
	{}

	thread_pool & get()
	{
		return TP;
	}

private:
	thread_pool TP;
};

//see documentation for thread_pool_CPU
class thread_pool_IO : private boost::noncopyable
{
public:
	/*
	Pass in NULL for non-member func use.
	Note: Default NULL parameter not used because it would be error prone. It'd
		be easy to forget to specify object in initializer list if default
		initialization allowed.
	*/
	thread_pool_IO(void * object_ptr_in):
		object_ptr(object_ptr_in),
		stopped(false)
	{}

	~thread_pool_IO()
	{
		join();
	}

	void clear()
	{
		thread_pool_global_IO::singleton().get().clear(object_ptr);
	}

	bool enqueue(const boost::function<void ()> & func)
	{
		if(stopped){
			return false;
		}else{
			return thread_pool_global_IO::singleton().get().enqueue(func, object_ptr);
		}
	}

	bool enqueue(const boost::function<void ()> & func, const unsigned delay_ms)
	{
		if(stopped){
			return false;
		}else{
			return thread_pool_global_IO::singleton().get().enqueue(func, delay_ms, object_ptr);
		}
	}

	void join()
	{
		thread_pool_global_IO::singleton().get().join(object_ptr);
	}

	unsigned size()
	{
		return thread_pool_global_IO::singleton().get().size();
	}

	void start()
	{
		stopped = false;
	}

	void stop()
	{
		stopped = true;
	}

private:
	void * object_ptr;
	atomic_bool stopped;
};
#endif
