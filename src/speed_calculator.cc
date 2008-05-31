#include "speed_calculator.h"

speed_calculator::speed_calculator(unsigned int speed_limit_in)
:
speed_limit(speed_limit_in),
average_speed(0),
rate_control_count(0),
rate_control_damper(0),
rate_control_try_speed(0),
rate_control_try_time(time(0))
{

}

unsigned int speed_calculator::get_speed_limit()
{
	return speed_limit;
}

void speed_calculator::set_speed_limit(const unsigned int & new_speed_limit)
{
	speed_limit = new_speed_limit;
}

unsigned int speed_calculator::speed()
{
	return average_speed;
}

void speed_calculator::update(const unsigned int & byte_count)
{
	/*
	This function keeps track of how many bytes are downloaded in one second. It
	has a vector that keeps track of bytes in the current second and bytes in the
	previous second. Because the current second will always be incomplete, the
	previous second will be considered the download speed.
	*/
	unsigned int current_time = time(0);

	if(Second_Bytes.empty()){
		Second_Bytes.push_front(std::make_pair(current_time, 0));
	}

	if(Second_Bytes.front().first != current_time){
		//on a new second
		Second_Bytes.push_front(std::make_pair(current_time, byte_count));
	}else{
		Second_Bytes.front().second += byte_count;
	}

	//the current second is never counted in the average because it's incomplete
	if(Second_Bytes.size() > global::SPEED_AVERAGE + 1){
		Second_Bytes.pop_back();
	}

	unsigned int total_bytes = 0;
	std::deque<std::pair<unsigned int, unsigned int> >::reverse_iterator iter_cur, iter_end;
	iter_cur = Second_Bytes.rbegin();
	iter_end = Second_Bytes.rend();
	int count = 0;
	while(iter_cur != iter_end && iter_cur->first != current_time){
		total_bytes += iter_cur->second;
		++count;
		++iter_cur;
	}

	if(count != 0){
		average_speed = total_bytes / count;
	}else{
		average_speed = 0;
	}
}

unsigned int speed_calculator::rate_control(int max_possible_transfer)
{
	++rate_control_count;
	unsigned int transfer = 0;

	/*
	Reset rate_control_damper if no download has been going for
	global::SPEED_AVERAGE seconds.
	*/
	if(average_speed == 0){
		rate_control_damper = 0;
	}

	//maximum possible transfer to keep average_speed under rate
	if(!Second_Bytes.empty()){
		if(Second_Bytes.front().second < speed_limit){
			transfer = speed_limit - Second_Bytes.front().second;
		}
	}

	if(transfer > max_possible_transfer){
		//max transfer that can be sent is max_possible_transfer
		transfer = max_possible_transfer;
	}

	/*
	By limiting the rate the select() loop is made to iterate more often. To
	compensate for the increased CPU usage this function has a variable sleep.

	This is run once per second to adjust how much this function sleeps.
	*/
	if(rate_control_try_time != time(0)){
		//there is a bias toward speeding up
		if(average_speed >= rate_control_try_speed){
			rate_control_damper -= 1;
		}else{
			rate_control_damper += 2;
		}

		rate_control_try_speed = average_speed;
		rate_control_try_time = time(0);
	}

	//damper = 1 means that there is always a sleep, it should not go below 1
	if(rate_control_damper < 1){
		rate_control_damper = 1;
	}else if(rate_control_damper > 100){
		//reasonable limit for rate_control_damper
		rate_control_damper = 100;
	}

	//sleep every rate_control_damper calls to this function
	if(rate_control_count % rate_control_damper == 0){
		usleep(1);
	}

	return transfer;
}
