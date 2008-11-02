#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"
#include "locking_smart_pointer.h"

//std
#include <cmath>
#include <ctime>
#include <deque>

class speed_calculator
{
public:
	speed_calculator(unsigned int speed_limit_in = 0);

	/*
	get_speed_limit - returns the speed limit
	rate_control    - returns how many bytes can be sent to maintain rate
	speed           - returns the speed averaged over the last global::SPEED_AVERAGE time
	                  interval, average doesn't include current second
	update          - adds a n_bytes to be factored in to average speed
	*/
	unsigned int get_speed_limit();
	unsigned int rate_control(int max_possible_transfer);
	void set_speed_limit(const unsigned int & new_speed_limit);
	unsigned int speed();
	void update(const unsigned int & n_bytes);

private:
	locking_smart_pointer<unsigned int> speed_limit;   //used by rate_control
	locking_smart_pointer<unsigned int> average_speed; //average speed over global::SPEED_AVERAGE seconds

	/*
	pair<second, bytes in second>
	The low elements are more current in time.
	*/
	std::pair<time_t, unsigned int> Second_Bytes[global::SPEED_AVERAGE + 1];

	//used by rate_control to control interval between usleep()'s
	unsigned long rate_control_count;
};
#endif
