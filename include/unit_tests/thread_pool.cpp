/*
This is a poor test of the thread pool. It's hard to do a unit test for this.
*/

//include
#include <logger.hpp>
#include <portable_sleep.hpp>
#include <thread_pool.hpp>

void func()
{
	portable_sleep::ms(200);
}

int main()
{
	thread_pool Thread_Pool(2);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.join();
}
