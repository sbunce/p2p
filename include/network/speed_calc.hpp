#ifndef H_NETWORK_SPEED_CALCULATOR
#define H_NETWORK_SPEED_CALCULATOR

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <ctime>
#include <deque>

namespace network{
class speed_calc : private boost::noncopyable
{
	//must be >= 1
	static const unsigned AVERAGE = 8;

public:
	speed_calc();

	/*
	add:
		Add bytes to the average.
	current_second:
		Returns how many bytes have been seen in the current second.
	speed:
		Returns average speed for the previous AVERAGE seconds. This does not
		include the bytes for the current incomplete second.

	*/
	void add(const unsigned n_bytes);
	unsigned current_second();
	unsigned speed();

private:
	//mutex for all public functions
	boost::mutex Mutex;

	//most recently calculated average speed
	unsigned average_speed;

	/*
	The 0 element is the most recent second, the last element is the least
	recent second. All seconds are associate with a number (of bytes or some
	other quantity) seen in that second.
	*/
	std::pair<std::time_t, unsigned> Second[AVERAGE + 1];

	/*
	add_prive:
		Add bytes in to the average.
		Precondition: Mutex must be locked.
	*/
	void add_priv(const unsigned n_bytes);
};
}
#endif
