//THREADSAFE
#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <ctime>
#include <deque>

namespace network{
class speed_calculator : private boost::noncopyable
{
public:
	//must be >= 1
	static const unsigned AVERAGE = 4;

	speed_calculator():
		average_speed(0)
	{
		for(int x=0; x<AVERAGE + 1; ++x){
			Second[x].first = Second[x].second = 0;
		}
	}

	//returns how much has been seen in the current second
	unsigned current_second()
	{
		boost::mutex::scoped_lock lock(Mutex);
		add_priv(0);
		return Second[0].second;
	}

	/*
	Returns average speed for the previous AVERAGE seconds. This does not include
	the bytes for the current incomplete second.
	*/
	unsigned speed()
	{
		boost::mutex::scoped_lock lock(Mutex);
		add_priv(0);
		return average_speed;
	}

	//add n_bytes to the average
	void add(const unsigned n_bytes)
	{
		boost::mutex::scoped_lock lock(Mutex);
		add_priv(n_bytes);
	}

private:
	//mutex for all public functions
	boost::mutex Mutex;

	//most recently calculated average speed, updated by update()
	unsigned average_speed;

	/*
	The 0 element is the most recent second, the last element is the least
	recent second. All seconds are associate with a number (of bytes or some
	other quantity) seen in that second.
	*/
	std::pair<std::time_t, unsigned> Second[AVERAGE + 1];

	/*
	Add bytes in to the average.
	Precondition: Mutex must be locked.
	*/
	void add_priv(const unsigned n_bytes)
	{
		std::time_t current_second = std::time(NULL);
		if(Second[0].first == current_second){
			//most recent second is already current second
			Second[0].second += n_bytes;
		}else{
			//most recent second is not current second
			unsigned shift = current_second - Second[0].first;
			if(shift > AVERAGE){
				//most recent second too old for shifting, make as new
				for(int x=1; x<AVERAGE + 1; ++x){
					Second[x].first = Second[x].second = 0;
				}
				Second[0].first = current_second;
				Second[0].second = n_bytes;
			}else{
				//a shift can be done
				for(int x=AVERAGE - shift; x>=0; --x){
					Second[x + shift] = Second[x];
				}
				//fill in any gap
				for(int x=0; x<shift; ++x){
					Second[x].first = current_second - x;
					Second[x].second = 0;
				}
				Second[0].second += n_bytes;
			}
		}

		//update the average_speed
		average_speed = 0;
		for(int x=1; x<AVERAGE + 1; ++x){
			average_speed += Second[x].second;
		}
		average_speed /= AVERAGE;
	}
};
}
#endif
