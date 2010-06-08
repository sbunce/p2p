#ifndef H_NET_SPEED_CALC
#define H_NET_SPEED_CALC

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <ctime>
#include <vector>

namespace net{
class speed_calc : private boost::noncopyable
{
public:
	//seconds to average over (1 to 180)
	speed_calc(const unsigned average_seconds_in = 8);

	/*
	add:
		Add bytes to the average.
	current_second:
		Returns how many bytes have been seen in the current second.
	speed:
		Returns average speed. This does not include the bytes for the current
		incomplete second.
	*/
	void add(const unsigned n_bytes);
	unsigned current_second();
	unsigned speed();

private:
	//mutex for all public functions
	boost::mutex Mutex;

	//maximum number of seconds to average over
	const unsigned average_seconds;

	//most recently calculated average speed
	unsigned average_speed;

	/*
	The 0 element is the most recent second, the last element is the least
	recent second. All seconds are associate with a number (of bytes or some
	other quantity) seen in that second.
	*/
	std::vector<std::pair<std::time_t, unsigned> > Second;

	/*
	add_prive:
		Add bytes in to the average.
		Precondition: Mutex must be locked.
	*/
	void add_priv(const unsigned n_bytes);
};
}//end of namespace net
#endif
