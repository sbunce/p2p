#include "thread_pool.h"

thread_pool::thread_pool(
	const int & thread_limit_in
):
	thread_limit(thread_limit_in)
{

}

thread_pool::~thread_pool()
{
	thread_limit = 0;
	Pool.interrupt_all();
	Pool.join_all();
}

void thread_pool::join()
{
	Pool.join_all();
}

void thread_pool::max(int threads)
{
	thread_limit = threads;
}

void thread_pool::process(boost::thread * T)
{
	boost::function0<void> func;
	while(true){
		boost::this_thread::interruption_point();
		{//begin lock scope
		boost::mutex::scoped_lock lock(work_queue_mutex);
		if(work_queue.empty()){
			//no jobs, terminate thread
			break;
		}
		func = work_queue.front();
		work_queue.pop();
		}//end lock scope
		func();
	}
	Pool.remove_thread(T);
	delete T;
}

void thread_pool::queue(const boost::function0<void> & func)
{
	boost::mutex::scoped_lock lock(work_queue_mutex);
	work_queue.push(func);
	if(Pool.size() < thread_limit){
		boost::thread * T = new boost::thread;
		*T = boost::thread(boost::bind(&thread_pool::process, this, T));
		Pool.add_thread(T);
	}
}
