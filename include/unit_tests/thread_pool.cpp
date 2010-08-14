//include
#include <atomic_int.hpp>
#include <logger.hpp>
#include <thread_pool.hpp>
#include <unit_test.hpp>

int fail(0);

void inc(atomic_int<unsigned> & cnt)
{
	++cnt;
}

int main()
{
	unit_test::timeout();
	thread_pool TP;
	atomic_int<unsigned> cnt(0);
	for(unsigned x=0; x<32; ++x){
		TP.enqueue(boost::bind(&inc, boost::ref(cnt)));
	}
	TP.join();
	if(cnt != 32){
		LOG; ++fail;
	}
	return fail;
}
