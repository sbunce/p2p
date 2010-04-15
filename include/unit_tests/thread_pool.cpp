//include
#include <atomic_int.hpp>
#include <logger.hpp>
#include <thread_pool.hpp>

int fail(0);
atomic_int<unsigned> cnt(0);

void inc_cnt()
{
	++cnt;
}

int main()
{
	thread_pool TP;
	for(int x=0; x<1024; ++x){
		TP.enqueue(inc_cnt);
	}
	TP.join();
	if(cnt != 1024){
		LOGGER(logger::utest); ++fail;
	}
	return fail;
}
