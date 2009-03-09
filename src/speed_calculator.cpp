#include "speed_calculator.hpp"

speed_calculator::speed_calculator()
:
	Recursive_Mutex(new boost::recursive_mutex()),
	average(0)
{
	Second_Bytes[0] = std::make_pair(std::time(NULL), 0);
	for(int x=1; x < settings::SPEED_AVERAGE + 1; ++x){
		Second_Bytes[x] = std::make_pair(0, 0);
	}
}

unsigned speed_calculator::current_second_bytes()
{
	boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
	update(0);
	return Second_Bytes[0].second;
}

unsigned speed_calculator::speed()
{
	boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
	update(0);
	return average;
}

void speed_calculator::update(const unsigned & n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
	time_t current_time = time(NULL);

	if(Second_Bytes[0].first == current_time){
		//add bytes to current second
		Second_Bytes[0].second += n_bytes;
	}else{
		/*
		The most recent second in Second_Bytes is not the current second. Do right
		shifts and inject new seconds on the left.
		*/
		while(Second_Bytes[0].first != current_time){
			for(int x=settings::SPEED_AVERAGE; x > 0; --x){
				Second_Bytes[x] = Second_Bytes[x-1];
			}
			Second_Bytes[0] = std::make_pair(Second_Bytes[1].first + 1, 0);
		}
		Second_Bytes[0].second += n_bytes;
	}

	//calculate average, don't include first incomplete second
	unsigned total_bytes = 0, average_seconds = 0;
	for(int x=1; x < settings::SPEED_AVERAGE + 1; ++x){
		if(Second_Bytes[x].first != 0){
			total_bytes += Second_Bytes[x].second;
			++average_seconds;
		}
	}
	if(average_seconds != 0){
		average = total_bytes / average_seconds;
	}else{
		average = 0;
	}
}
