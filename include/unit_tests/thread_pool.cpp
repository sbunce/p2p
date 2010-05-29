//include
#include <atomic_int.hpp>
#include <logger.hpp>
#include <thread_pool.hpp>
#include <unit_test.hpp>

int fail(0);
atomic_int<unsigned> cnt(0);

void inc_cnt()
{
	++cnt;
}

int main()
{
	unit_test::timeout();

	//schedule jobs to increment cnt
	thread_pool TP;
	for(int x=0; x<1024; ++x){
		TP.enqueue(inc_cnt);
	}
	//wait for jobs to finish
	TP.join();
	if(cnt != 1024){
		LOG; ++fail;
	}
	return fail;
}
