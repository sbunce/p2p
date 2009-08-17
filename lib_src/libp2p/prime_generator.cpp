#include "prime_generator.hpp"

prime_generator::prime_generator()
{
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&prime_generator::main_loop, this));
	}
}

prime_generator::~prime_generator()
{
	Workers.interrupt_all();
	Workers.join_all();
}

void prime_generator::main_loop()
{
	/*
	Thread takes a lot of CPU time to bring up. This can increase the time it
	takes to instantiate libp2p. This delay speeds up library instantiation. This
	is important for fast GUI startup time.
	*/
	portable_sleep::yield();

	KISS PRNG;
	PRNG.seed();
	mpint random_prime;
	while(true){
		boost::this_thread::interruption_point();
		{//begin lock scope
		boost::mutex::scoped_lock lock(prime_mutex);
		while(Prime_Cache.size() >= settings::PRIME_CACHE){
			prime_generate_cond.wait(prime_mutex);
		}
		}//end lock scope

		//this should not be locked
		random_prime = mpint::random_prime_fast(protocol::DH_KEY_SIZE, PRNG);

		{//begin lock scope
		boost::mutex::scoped_lock lock(prime_mutex);
		/*
		With multiple threads it's possible that we can go beyond the limit when
		multiple threads generate primes at the same time. Hence the check.
		*/
		if(Prime_Cache.size() < settings::PRIME_CACHE){
			Prime_Cache.push_back(random_prime);
		}
		}//end lock scope

		//notify possible threads waiting for prime to be generated
		prime_remove_cond.notify_one();
	}
}

unsigned prime_generator::prime_count()
{
	boost::mutex::scoped_lock lock(prime_mutex);
	return Prime_Cache.size();
}

mpint prime_generator::random_prime()
{
	mpint temp;
	boost::mutex::scoped_lock lock(prime_mutex);
	while(Prime_Cache.empty()){
		prime_remove_cond.wait(prime_mutex);
	}
	temp = Prime_Cache.back();
	Prime_Cache.pop_back();
	prime_generate_cond.notify_one();
	return temp;
}
