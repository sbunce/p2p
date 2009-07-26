#include "prime_generator.hpp"

prime_generator::prime_generator()
{

}

prime_generator::~prime_generator()
{
	database::table::prime::write_all(Prime_Cache);
}

void prime_generator::main_loop()
{
	//delay allows p2p_real to be constructed faster
	portable_sleep::ms(100);

	{//begin lock scope
	boost::mutex::scoped_lock lock(prime_mutex);
	static bool read_in = false;
	if(!read_in){
		database::table::prime::read_all(Prime_Cache);
		read_in = true;
	}
	}//end lock scope

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
		random_prime = mpint::random_prime(protocol::DH_KEY_SIZE);

		{//begin lock scope
		boost::mutex::scoped_lock lock(prime_mutex);
		/*
		With multiple threads it's possible that we can go beyond the limit when
		multiple threads generate primes at the same time when one below the prime
		limit.
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

void prime_generator::start()
{
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&prime_generator::main_loop, this));
	}
}

void prime_generator::stop()
{
	Workers.interrupt_all();
	Workers.join_all();
}
