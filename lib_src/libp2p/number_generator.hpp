//THREADSAFE, SINGLETON, THREAD SPAWNING
#ifndef H_NUMBER_GENERATOR
#define H_NUMBER_GENERATOR

//custom
#include "database.hpp"
#include "settings.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <singleton.hpp>
#include <tommath/mpint.hpp>

//standard
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

//windows PRNG
#ifdef WIN32
	#include <wincrypt.h>
#endif

class number_generator : public singleton_base<number_generator>
{
	friend class singleton_base<number_generator>;
public:
	//called by p2p dtor to terminate generate thread
	void stop();

	/*
	prime_count:
		Returns number of primes in prime_cache.
	random:
		Returns random mpint of specified size.
	random_prime:
		Returns prime in prime cache. Size of prime is always settings::DH_KEY_SIZE.
	*/
	unsigned prime_count();
	mpint random(const int & bytes);
	mpint random_prime();

private:
	number_generator();

	boost::thread generate_thread;

	boost::condition_variable_any prime_remove_cond;
	boost::condition_variable_any prime_generate_cond;
	boost::mutex prime_mutex;

	//cache of primes, all access must be locked with prime_mutex
	std::vector<mpint> Prime_Cache;

	/*
	PRNG function that mp_prime_random_ex needs
		buff is filled with random bytes
		length is how many bytes to put in to buff
		data is optional pointer
	*/
	static int PRNG(unsigned char * buff, int length, void * data);

	/*
	generate:
		The generate_thread runs in this function and generates primes.
	*/
	void generate();
};
#endif
