#include "load_scanner.hpp"

load_scanner::load_scanner():
	stop_flag(false)
{
	global_thread_pool::singleton().IO().enqueue(
		boost::bind(&load_scanner::scan, this), this);
}

load_scanner::~load_scanner()
{
	stop_flag = true;
	global_thread_pool::singleton().IO().clear(this);
	global_thread_pool::singleton().IO().join(this);
}

void load_scanner::scan()
{
	//LOG;

	if(!stop_flag){
		//call again in 1 second
		global_thread_pool::singleton().IO().enqueue(
			boost::bind(&load_scanner::scan, this), 1000, this);
	}
}
