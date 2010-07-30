#include "prime_generator.hpp"

prime_generator::prime_generator()
{
	for(unsigned x=0; x<settings::PRIME_CACHE; ++x){
		Thread_Pool.enqueue(boost::bind(&prime_generator::generate, this));
	}
}

prime_generator::~prime_generator()
{
	Thread_Pool.stop();
	Thread_Pool.clear();
}

void prime_generator::generate()
{
	mpa::mpint p = mpa::random_prime(protocol_tcp::DH_key_size);
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Cache.push_back(p);
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
	//replace prime we took from cache
	Thread_Pool.enqueue(boost::bind(&prime_generator::generate, this));
	return tmp;
}
