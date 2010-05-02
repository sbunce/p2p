#include <network/network.hpp>

network::speed_calc::speed_calc(
	const unsigned average_seconds_in
):
	average_seconds(average_seconds_in),
	average_speed(0)
{
	assert(average_seconds != 0);
	assert(average_seconds <= 180);
	Second.resize(average_seconds + 1);
	for(int x=0; x<average_seconds + 1; ++x){
		Second[x].first = Second[x].second = 0;
	}
}

void network::speed_calc::add(const unsigned n_bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	add_priv(n_bytes);
}

void network::speed_calc::add_priv(const unsigned n_bytes)
{
	std::time_t current_second = std::time(NULL);
	if(Second[0].first == current_second){
		//most recent second is already current second
		Second[0].second += n_bytes;
	}else{
		//most recent second is not current second
		unsigned shift = current_second - Second[0].first;
		if(shift > average_seconds){
			//most recent second too old for shifting, make as new
			for(int x=1; x<average_seconds + 1; ++x){
				Second[x].first = Second[x].second = 0;
			}
			Second[0].first = current_second;
			Second[0].second = n_bytes;
		}else{
			//a shift can be done
			for(int x=average_seconds - shift; x>=0; --x){
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
	for(int x=1; x<average_seconds + 1; ++x){
		average_speed += Second[x].second;
	}
	average_speed /= average_seconds;
}

unsigned network::speed_calc::current_second()
{
	boost::mutex::scoped_lock lock(Mutex);
	add_priv(0);
	return Second[0].second;
}

unsigned network::speed_calc::speed()
{
	boost::mutex::scoped_lock lock(Mutex);
	add_priv(0);
	return average_speed;
}
