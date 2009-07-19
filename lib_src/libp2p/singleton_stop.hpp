#ifndef H_SINGLETON_STOP
#define H_SINGLETON_STOP

//custom
#include "database.hpp"
#include "prime_generator.hpp"
#include "share.hpp"

class singleton_stop
{
public:
	~singleton_stop()
	{
		prime_generator::singleton().stop();
		share::singleton().stop();
		thread_pool::singleton().stop();
	}
};
#endif
