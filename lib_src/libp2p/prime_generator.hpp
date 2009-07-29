//THREADSAFE, SINGLETON, THREAD SPAWNING
#ifndef H_PRIME_GENERATOR
#define H_PRIME_GENERATOR

//custom
#include "database.hpp"
#include "protocol.hpp"
#include "settings.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <CMWC4096.hpp>
#include <random.hpp>
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

class prime_generator
{
public:
	/*
	start:
		Starts prime generation threads.
		Note: This is only to be used from singleton_start.
	stop:
		Stops prime generation threads.
		Note: This is only to be used from singleton_stop.
	*/
	void start();
	void stop();

	/*
	prime_count:
		Returns number of primes in prime_cache.
	random_prime:
		Returns prime in prime cache. Size of prime is always settings::DH_KEY_SIZE.
	*/
	unsigned prime_count();
	mpint random_prime();

private:
	boost::thread_group Workers;

	/*
	The prime_remove_cond triggers the generate_thread to make more primes. The
	prime_generate_cond triggers threads waiting on primes to check to see if
	there is a prime to get. The prime_mutex locks all access to Prime_Cache.
	*/
	boost::condition_variable_any prime_remove_cond;
	boost::condition_variable_any prime_generate_cond;
	boost::mutex prime_mutex;

	//cache of primes, all access must be locked with prime_mutex
	std::vector<mpint> Prime_Cache;

	/*
	main_loop:
		Worker threads loop in this thread and generate primes.
	*/
	void main_loop();
};
#endif
