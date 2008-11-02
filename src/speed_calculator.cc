#include "speed_calculator.h"

speed_calculator::speed_calculator(
	unsigned int speed_limit_in
):
	speed_limit(new unsigned int(speed_limit_in)),
	average_speed(new unsigned int(0))
{
	for(int x=0; x < global::SPEED_AVERAGE + 1; ++x){
		Second_Bytes[x] = std::make_pair(time(NULL), 0);
	}
}

unsigned int speed_calculator::get_speed_limit()
{
	return **speed_limit;
}

void speed_calculator::set_speed_limit(const unsigned int & new_speed_limit)
{
	**speed_limit = new_speed_limit;
}

unsigned int speed_calculator::speed()
{
	return **average_speed;
}

void speed_calculator::update(const unsigned int & n_bytes)
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
	unsigned int count = 0;
	for(int x=1; x < global::SPEED_AVERAGE + 1; ++x){
		count += Second_Bytes[x].second;
	}
	**average_speed = count / global::SPEED_AVERAGE;
}

unsigned int speed_calculator::rate_control(int max_possible_transfer)
{
	unsigned int transfer = 0;

	while(Second_Bytes[0].second >= **speed_limit){
		//sleep until bytes can be requested
		portable_sleep::yield();

		//recalculate speed
		update(0);
	}

	//determine max bytes that can be transferred while staying within speed_limit
	transfer = **speed_limit - Second_Bytes[0].second;
	if(transfer > max_possible_transfer){
		transfer = max_possible_transfer;
	}

	return transfer;
}
