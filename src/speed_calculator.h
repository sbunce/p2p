#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

class speed_calculator
{
public:
	speed_calculator();

	/*
	rate_control - returns how many bytes are needed in the fractional second to be at specified rate
	               this function contains a usleep
	reset        - resets speed to zero
	speed        - returns the speed averaged over the last global::SPEED_AVERAGE time
	               interval, average doesn't include current second
	update       - adds a byte count to the average
	*/
	unsigned int rate_control(const int & rate, const int & max_possible_transfer);
	void reset();
	unsigned int speed();
	void update(const unsigned int & byte_count);

private:
	//pair<second, bytes in second>
	std::deque<std::pair<unsigned int, unsigned int> > Second_Bytes;

	unsigned int average_speed;
	boost::mutex AS_mutex;

	//used by rate control to keep track of maximum send/receive in a time interval
	int transfer_limit;
};
#endif
