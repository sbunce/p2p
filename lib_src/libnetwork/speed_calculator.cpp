#include <network/network.hpp>

network::speed_calculator::speed_calculator():
	average_speed(0)
{
	for(int x=0; x<AVERAGE + 1; ++x){
		Second[x].first = Second[x].second = 0;
	}
}

void network::speed_calculator::add(const unsigned n_bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	add_priv(n_bytes);
}

void network::speed_calculator::add_priv(const unsigned n_bytes)
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

unsigned network::speed_calculator::current_second()
{
	boost::mutex::scoped_lock lock(Mutex);
	add_priv(0);
	return Second[0].second;
}

unsigned network::speed_calculator::speed()
{
	boost::mutex::scoped_lock lock(Mutex);
	add_priv(0);
	return average_speed;
}
