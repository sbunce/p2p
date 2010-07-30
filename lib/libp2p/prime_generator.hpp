#ifndef H_PRIME_GENERATOR
#define H_PRIME_GENERATOR

//custom
#include "db_all.hpp"
#include "protocol_tcp.hpp"
#include "settings.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <mpa.hpp>
#include <RC4.hpp>
#include <thread_pool.hpp>

//standard
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>

class prime_generator : public singleton_base<prime_generator>
{
	friend class singleton_base<prime_generator>;
public:
	~prime_generator();

	/*
	random_prime:
		Returns random prime of size settings::DH_KEY_SIZE.
	*/
	mpa::mpint random_prime();

private:
	prime_generator();

	/*
	mutex:
		Locks the Prime_Cache container.
	cond:
		Notified when a prime removed from the cache.
	*/
	boost::mutex Mutex;
	boost::condition_variable_any Cond;
	std::list<mpa::mpint> Cache;

	/*
	generate:
		Generates a prime and puts it in the cache.
		Worker threads loop in this thread and generate primes.
	*/
	void generate();

	/*
	We don't use thread_pool_global because it would cause deadlock potential.
	It's not very necessary to use it here anyways since there will only be one
	prime_generator instantiated.
	*/
	thread_pool Thread_Pool;
};
#endif
