#include "number_generator.h"

number_generator * number_generator::Number_Generator = NULL;
boost::mutex number_generator::Mutex;

number_generator::number_generator()
{
	boost::thread T(boost::bind(&number_generator::generate_primes_thread, this));
}

unsigned int number_generator::prime_count_priv()
{
	return DB_Prime.count();
}

mpint number_generator::random_mpint_priv(const int & bytes)
{
	unsigned char buff[bytes];
	PRNG(buff, bytes, NULL);
	return mpint(buff, bytes);
}

mpint number_generator::random_prime_mpint_priv()
{
	mpint m;
	while(!DB_Prime.retrieve(m)){
		//no prime available, wait for one
		portable_sleep::yield();
	}
	return m;
}

void number_generator::generate_primes_thread()
{
	while(true){
		bool do_sleep = false;
		if(DB_Prime.count() >= global::PRIME_CACHE){
			do_sleep = true;
		}

		if(do_sleep){
			//enough primes generated, sleep for a while
			portable_sleep::ms(1000);
			continue;
		}

		mpint m;
		mp_prime_random_ex(
			&m.data,               //mp_int structure
			1,                     //Miller-Rabin tests
			global::DH_KEY_SIZE*8, //size (bits) of prime to generate
			0,                     //optional flags
			&PRNG,
			NULL                   //optional void* that can be passed to PRNG
		);

		DB_Prime.add(m);
	}
}
