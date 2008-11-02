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
	Pool.join_all();
}

void thread_pool::process()
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
}

void thread_pool::queue(const boost::function0<void> & func)
{
	boost::mutex::scoped_lock lock(work_queue_mutex);
	work_queue.push(func);
	if(Pool.size() < thread_limit){
		Pool.create_thread(boost::bind(&thread_pool::process, this));
	}
}
