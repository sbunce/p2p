//THREADSAFE
#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//boost
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

//custom
#include "atomic_int.hpp"
#include "global.hpp"

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
	//mutex for all public functions
	boost::shared_ptr<boost::recursive_mutex> Recursive_Mutex;

	//average speed over global::SPEED_AVERAGE seconds
	unsigned average;

	/*
	pair<second, bytes in second>
	The low elements are more current in time.
	*/
	std::pair<time_t, unsigned> Second_Bytes[global::SPEED_AVERAGE + 1];
};
#endif
