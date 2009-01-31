#include "speed_calculator.hpp"

speed_calculator::speed_calculator()
: average(0)
{
	for(int x=0; x < global::SPEED_AVERAGE + 1; ++x){
		Second_Bytes[x] = std::make_pair(time(NULL), 0);
	}
}

unsigned speed_calculator::current_second_bytes()
{
	update(0);
	return Second_Bytes[0].second;
}

unsigned speed_calculator::speed()
{
	update(0);
	return average;
}

void speed_calculator::update(const unsigned & n_bytes)
{
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
			//right shift
			for(int x=global::SPEED_AVERAGE; x > 0; --x){
				Second_Bytes[x] = Second_Bytes[x-1];
			}
			//inject
			Second_Bytes[0] = std::make_pair(Second_Bytes[1].first + 1, 0);
		}

		//add bytes to new second
		Second_Bytes[0].second += n_bytes;
	}

	//calculate average, don't include first incomplete second
	unsigned count = 0;
	for(int x=1; x < global::SPEED_AVERAGE + 1; ++x){
		count += Second_Bytes[x].second;
	}
	average = count / global::SPEED_AVERAGE;
}
