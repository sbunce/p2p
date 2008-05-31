#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

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
	update          - adds a byte count to the average
	*/
	unsigned int get_speed_limit();
	unsigned int rate_control(int max_possible_transfer);
	void set_speed_limit(const unsigned int & new_speed_limit);
	unsigned int speed();
	void update(const unsigned int & byte_count);

private:
	volatile unsigned int speed_limit;

	//pair<second, bytes in second>
	std::deque<std::pair<unsigned int, unsigned int> > Second_Bytes;
	volatile unsigned int average_speed;

	//used by rate_control to control interval between usleep()'s
	unsigned long rate_control_count;
	int rate_control_damper;
	unsigned int rate_control_try_speed;
	unsigned long rate_control_try_time;
};
#endif
