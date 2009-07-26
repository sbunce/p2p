#ifndef H_SINGLETON_STOP
#define H_SINGLETON_STOP

//custom
#include "database.hpp"
#include "thread_pool.hpp"

class singleton_stop
{
public:
	~singleton_stop()
	{
		thread_pool::singleton().stop();
	}
};
#endif
