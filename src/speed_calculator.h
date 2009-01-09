#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "atomic_int.h"
#include "global.h"

//std
#include <cmath>
#include <ctime>
#include <deque>

class speed_calculator
{
public:
	speed_calculator();

	/*
	current_second_bytes - returns how many bytes have been received in the current second
	speed                - returns average speed
	update               - add n_bytes to the average
	*/
	unsigned current_second_bytes();
	unsigned speed();
	void update(const unsigned & n_bytes);

private:
	atomic_int<unsigned> average;

	/*
	pair<second, bytes in second>
	The low elements are more current in time.
	*/
	std::pair<time_t, atomic_int<unsigned> > Second_Bytes[global::SPEED_AVERAGE + 1];
};
#endif
