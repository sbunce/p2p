#ifndef H_GLOBAL_THREAD_POOL
#define H_GLOBAL_THREAD_POOL

//include
#include <singleton.hpp>
#include <thread_pool.hpp>

class global_thread_pool : public singleton_base<global_thread_pool>
{
	friend class singleton_base<global_thread_pool>;
public:
	global_thread_pool():
		IO_TP(4)
	{

	}

	//used for CPU bound jobs
	thread_pool & CPU()
	{
		return CPU_TP;
	}

	//used for IO bound jobs
	thread_pool & IO()
	{
		return IO_TP;
	}

private:
	thread_pool CPU_TP;
	thread_pool IO_TP;
};
#endif
