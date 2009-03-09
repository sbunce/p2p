//include
#include <logger.hpp>
#include <thread_pool.hpp>

void func()
{
	sleep(1);
}

int main()
{
	thread_pool Thread_Pool(4);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.queue_job(&func);
	Thread_Pool.join();
}
