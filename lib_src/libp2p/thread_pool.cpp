#include "thread_pool.hpp"

thread_pool::thread_pool():
	stop_threads(false)
{

}

void thread_pool::dispatcher()
{
	boost::function0<void> CB;
	while(true){
		{//begin lock scope
		boost::mutex::scoped_lock lock(call_back_mutex);
		while(call_back.empty()){
			//only stop thread when there are no more jobs
			if(stop_threads){
				return;
			}
			call_back_cond.wait(call_back_mutex);
		}
		CB = call_back.front();
		call_back.pop_front();
		}//end lock scope
		CB();
	}
}

void thread_pool::queue(const boost::function0<void> & CB)
{
	boost::mutex::scoped_lock lock(call_back_mutex);
	call_back.push_back(CB);
	call_back_cond.notify_one();
}

void thread_pool::start()
{
	for(int x=0; x<settings::THREAD_POOL_SIZE; ++x){
		Workers.create_thread(boost::bind(&thread_pool::dispatcher, this));
	}
}

void thread_pool::stop()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(call_back_mutex);
	stop_threads = true;
	}//end lock scope
	call_back_cond.notify_all();
	Workers.join_all();
}
