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
		timeout_ms(0)
	{
		for(unsigned x=0; x<threads; ++x){
			workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
		}
	}

	~thread_pool()
	{
		//wait for all jobs to finish
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(mutex);
		while(!job_obj.empty()){
			join_cond.wait(mutex);
		}
		}//END lock scope

		timeout_thread.interrupt();
		timeout_thread.join();
		workers.interrupt_all();
		workers.join_all();
	}

	//clear jobs
	void clear(void * const obj_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		for(std::list<job>::iterator it_cur = job_queue.begin(); it_cur != job_queue.end();){
			if(it_cur->obj_ptr == obj_ptr){
				std::map<void *, unsigned>::iterator it = job_obj.find(obj_ptr);
				assert(it != job_obj.end());
				if(it->second == 1){
					job_obj.erase(it);
				}else{
					--it->second;
				}
				it_cur = job_queue.erase(it_cur);
			}else{
				++it_cur;
			}
		}
		for(std::multimap<boost::uint64_t, job>::iterator it_cur = timeout_jobs.begin();
			it_cur != timeout_jobs.end();)
		{
			if(it_cur->second.obj_ptr == obj_ptr){
				std::map<void *, unsigned>::iterator it = job_obj.find(obj_ptr);
				assert(it != job_obj.end());
				if(it->second == 1){
					job_obj.erase(it);
				}else{
					--it->second;
				}
				timeout_jobs.erase(it_cur++);
			}else{
				++it_cur;
			}
		}
		join_cond.notify_all();
	}

	//enqueue job
	bool enqueue(const boost::function<void ()> & func, void * const obj_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		if(stopped){
			return false;
		}else{
			job_queue.push_back(job(func, obj_ptr));
			std::map<void *, unsigned>::iterator it = job_obj.find(obj_ptr);
			if(it == job_obj.end()){
				job_obj.insert(std::make_pair(obj_ptr, 1));
			}else{
				++it->second;
			}
			job_cond.notify_one();
			return true;
		}
	}

	//enqueue job after specified delay (ms)
	bool enqueue(const boost::function<void ()> & func, const unsigned delay_ms,
		void * const obj_ptr = NULL)
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
				job_queue.push_back(job(func, obj_ptr));
				job_cond.notify_one();
			}else{
				timeout_jobs.insert(std::make_pair(timeout_ms + delay_ms, job(func, obj_ptr)));
			}
			std::map<void *, unsigned>::iterator it = job_obj.find(obj_ptr);
			if(it == job_obj.end()){
				job_obj.insert(std::make_pair(obj_ptr, 1));
			}else{
				++it->second;
			}
			return true;
		}
	}

	//block until all jobs done
	void join(void * const obj_ptr = NULL)
	{
		boost::mutex::scoped_lock lock(mutex);
		while(job_obj.find(obj_ptr) != job_obj.end()){
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
	class job
	{
	public:
		job(
			const boost::function<void ()> & func_in = boost::function<void ()>(),
			void * const obj_ptr_in = NULL
		):
			func(func_in),
			obj_ptr(obj_ptr_in)
		{}
		boost::function<void ()> func;
		void * obj_ptr;
	};

	boost::mutex mutex;                      //locks everyting (object is a monitor)
	boost::thread_group workers;             //dispatcher() threads
	boost::thread timeout_thread;            //timout_dispatcher() thread
	boost::condition_variable_any job_cond;  //notified when job added
	boost::condition_variable_any join_cond; //notified when job added or job finished
	bool stopped;                            //when true no jobs can be added
	boost::uint64_t timeout_ms;              //incremented by timeout_thread

	//new jobs pushed on back, jobs on front processed
	std::list<job> job_queue;
	//jobs enqueued after timeout, when timeout_ms >= key
	std::multimap<boost::uint64_t, job> timeout_jobs;
	//obj_ptrs for queued and running jobs (std::map<obj_ptr, # of jobs>)
	std::map<void *, unsigned> job_obj;

	//consumes enqueued jobs, does call backs
	void dispatcher()
	{
		while(true){
			job tmp;
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			while(job_queue.empty()){
				job_cond.wait(mutex);
			}
			tmp = job_queue.front();
			job_queue.pop_front();
			}//END lock scope
			tmp.func();
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(mutex);
			std::map<void *, unsigned>::iterator it = job_obj.find(tmp.obj_ptr);
			assert(it != job_obj.end());
			if(it->second == 1){
				job_obj.erase(it);
			}else{
				--it->second;
			}
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

/*
Thread pool that shares threads globally. The functions are equivalent to those
in thread_pool but they all work with the obj_ptr passed to the ctor.
note: instantiate at bottom of header so join() works correctly
*/
class thread_pool_global : private boost::noncopyable
{
	//number of threads in thread_pool_global
	const static unsigned pool_size = 8;

public:
	/*
	Pass in NULL if no object owns the thread_pool_global.
	Note: Default NULL parameter not used because it would be error prone. It'd
		be easy to forget to specify object in initializer list if default
		initialization allowed.
	*/
	explicit thread_pool_global(void * const obj_ptr_in):
		obj_ptr(obj_ptr_in),
		stopped(false),
		running_jobs(0),
		TPW_Singleton(thread_pool_wrap::singleton())
	{

	}

	~thread_pool_global()
	{
		join();
	}

	void clear()
	{
		boost::mutex::scoped_lock lock(Mutex);
		job_queue.clear();
		TPW_Singleton->TP.clear(obj_ptr);
	}

	bool enqueue(const boost::function<void ()> & func)
	{
		boost::mutex::scoped_lock lock(Mutex);
		if(stopped){
			return false;
		}else{
			if(running_jobs < pool_size){
				++running_jobs;
				//wrap the call back to potentially schedule other jobs
				return TPW_Singleton->TP.enqueue(
					boost::bind(&thread_pool_global::call_back_wrap, this, func), obj_ptr);
			}else{
				//schedule job
				job_queue.push_back(func);
				return true;
			}
		}
	}

	bool enqueue(const boost::function<void ()> & func, const unsigned delay_ms)
	{
		boost::mutex::scoped_lock lock(Mutex);
		if(stopped){
			return false;
		}else{
			if(delay_ms == 0){
				if(running_jobs < pool_size){
					++running_jobs;
					//wrap the call back to potentially schedule other jobs
					return TPW_Singleton->TP.enqueue(
						boost::bind(&thread_pool_global::call_back_wrap, this, func), obj_ptr);
				}else{
					//schedule job
					job_queue.push_back(func);
					return true;
				}
			}else{
				//enqueue job after delay
				return TPW_Singleton->TP.enqueue(boost::bind(
					&thread_pool_global::enqueue, this, func), delay_ms, obj_ptr);
			}
		}
	}

	void join()
	{
		TPW_Singleton->TP.join(obj_ptr);
	}

	void start()
	{
		boost::mutex::scoped_lock lock(Mutex);
		stopped = false;
	}

	void stop()
	{
		boost::mutex::scoped_lock lock(Mutex);
		stopped = true;
	}

private:
	//global thread_pool that contains # of thread = hardware threads
	class thread_pool_wrap : public singleton_base<thread_pool_wrap>
	{
		friend class singleton_base<thread_pool_global>;
	public:
		thread_pool_wrap():
			TP(pool_size)
		{}
		thread_pool TP;
	};

	boost::mutex Mutex;      //locks everything (object is a monitor)
	void * const obj_ptr; //passed to global thread pool
	bool stopped;            //if true thread_pool_global doesn't allow new jobs

	/*
	We don't want one object to hog the global thread pool so we maintain a
	job_queue specific to the object. We limit how many concurrent jobs we give
	to the global thread pool and put the overflow in this container.
	*/
	unsigned running_jobs; //# of jobs sent to global thread pool
	std::list<boost::function<void ()> > job_queue;

	/*
	Storing a shared_ptr to the thread_pool singleton allows us to instantiate
	this object within other singletons and avoid static initialization order
	problems. See documentation in singleton.hpp for more info.
	*/
	boost::shared_ptr<thread_pool_wrap> TPW_Singleton;

	/*
	This function wraps call backs from the global thread pool. This is used to
	schedule new jobs when our existing jobs finish.
	*/
	void call_back_wrap(const boost::function<void ()> func)
	{
		//run the thread_pool job
		func();

		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(Mutex);
		--running_jobs;
		if(running_jobs < pool_size && !job_queue.empty()){
			++running_jobs;
			boost::function<void ()> tmp = job_queue.front();
			job_queue.pop_front();
			TPW_Singleton->TP.enqueue(
				boost::bind(&thread_pool_global::call_back_wrap, this, tmp), obj_ptr);
		}
		}//END lock scope
	}
};
#endif
