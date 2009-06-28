/*
This container measures a speed. It can be used for measuring bytes per second,
queries per second, or other units per second.
*/
//THREADSAFE
#ifndef H_SPEED_CALCULATOR
#define H_SPEED_CALCULATOR

//boost
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

//std
#include <ctime>
#include <deque>

class speed_calculator
{
public:
	static const int average_seconds = 4;

	speed_calculator():
		Recursive_Mutex(new boost::recursive_mutex()),
		average(0)
	{
		Second_Bytes[0] = std::make_pair(std::time(NULL), 0);
		for(int x=1; x < average_seconds + 1; ++x){
			Second_Bytes[x] = std::make_pair(0, 0);
		}
	}

	//returns how much has been seen in the current second
	unsigned current_second()
	{
		boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
		update(0);
		return Second_Bytes[0].second;
	}

	//returns average speed		
	unsigned speed()
	{
		boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
		update(0);
		return average;
	}

	//add n_bytes to the average
	void update(const unsigned & n_bytes)
	{
		boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
		time_t current_time = std::time(NULL);

		if(Second_Bytes[0].first == current_time){
			//add bytes to current second
			Second_Bytes[0].second += n_bytes;
		}else{
			/*
			The most recent second in Second_Bytes is not the current second. Do right
			shifts and inject new seconds on the left.
			*/
			while(Second_Bytes[0].first != current_time){
				for(int x=average_seconds; x > 0; --x){
					Second_Bytes[x] = Second_Bytes[x-1];
				}
				Second_Bytes[0] = std::make_pair(Second_Bytes[1].first + 1, 0);
			}
			Second_Bytes[0].second += n_bytes;
		}

		//calculate average
		unsigned total_bytes = 0, counted_seconds = 0;
		for(int x=1; x < average_seconds + 1; ++x){
			if(Second_Bytes[x].first != 0){
				total_bytes += Second_Bytes[x].second;
				++counted_seconds;
			}
		}
		if(counted_seconds == 0){
			average = 0;
		}else{
			average = total_bytes / counted_seconds;
		}
	}

private:
	//mutex for all public functions
	boost::shared_ptr<boost::recursive_mutex> Recursive_Mutex;

	//average speed
	unsigned average;

	/*
	pair<second, bytes in second>
	The low elements are more current in time.
	*/
	std::pair<time_t, unsigned> Second_Bytes[average_seconds + 1];
};
#endif
