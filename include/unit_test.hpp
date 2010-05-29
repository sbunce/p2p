#ifndef H_UNIT_TEST
#define H_UNIT_TEST

//include
#include <boost/thread.hpp>
#include <logger.hpp>

namespace {
namespace unit_test
{

/*
Kill the program after the specified timeout. This stops a unit test from
hanging the build system if a unit test deadlocks or otherwise fails to
terminate properly.
*/
void timeout(const unsigned seconds = 8)
{
	struct func_local{
	static void timeout(const unsigned seconds)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000 * seconds));
		LOG << "timeout after " << seconds << " seconds";
		exit(1);
	}
	};
	boost::thread(boost::bind(&func_local::timeout, seconds));
}

}//end of unnamed namespace
}//end of namespace unit_test
#endif
