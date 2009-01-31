#include "number_generator.hpp"

number_generator * number_generator::Number_Generator = NULL;
boost::mutex number_generator::Mutex;

number_generator::number_generator()
{
	genprime_thread = boost::thread(boost::bind(&number_generator::genprime_loop, this));
}

unsigned number_generator::prime_count_priv()
{
	return DB_Prime.count();
}

mpint number_generator::random_mpint_priv(const int & bytes)
{
	assert(bytes <= global::DH_KEY_SIZE);
	unsigned char buff[global::DH_KEY_SIZE];
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

void number_generator::genprime_loop()
{
	while(true){
		if(DB_Prime.count() >= global::PRIME_CACHE){
			//enough primes generated, sleep for a while
			portable_sleep::ms(1000);
			continue;
		}

		mpint m;
		mp_prime_random_ex(
			&m.c_struct(),
			1,                     //Miller-Rabin tests
			global::DH_KEY_SIZE*8, //size (bits) of prime to generate
			0,                     //optional flags
			&PRNG,
			NULL                   //optional void* that can be passed to PRNG
		);
		DB_Prime.add(m);
	}
}
