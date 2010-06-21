#include "prime_generator.hpp"

prime_generator::prime_generator():
	TP_IO(this)
{
	TP_IO.enqueue(boost::bind(&prime_generator::generate, this));
}

prime_generator::~prime_generator()
{
	TP_IO.stop();
	TP_IO.clear();
}

void prime_generator::generate()
{
	mpa::mpint p = mpa::random_prime(protocol_tcp::DH_key_size);
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Cache.push_back(p);
	if(Cache.size() < settings::PRIME_CACHE_MIN){
		TP_IO.enqueue(boost::bind(&prime_generator::generate, this));
	}
	}//end lock scope
	Cond.notify_one();
}

mpa::mpint prime_generator::random_prime()
{
	boost::mutex::scoped_lock lock(Mutex);
	while(Cache.empty()){
		Cond.wait(Mutex);
	}
	mpa::mpint tmp = Cache.back();
	Cache.pop_back();
	TP_IO.enqueue(boost::bind(&prime_generator::generate, this));
	return tmp;
}
