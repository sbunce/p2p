//THREADSAFE, SINGLETON, THREAD SPAWNING
#ifndef H_NUMBER_GENERATOR
#define H_NUMBER_GENERATOR

//boost
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "database.hpp"
#include "global.hpp"

//libtommath
#include <mpint.hpp>

//std
#include <fstream>
#include <iostream>
#include <vector>

//windows PRNG
#ifdef WIN32
#include <wincrypt.h>
#endif

class number_generator : private boost::noncopyable
{
public:

	/*
	init:
		Initialize singleton. Not necessary to call but can be called to
		make sure prime generation thread is running.
	prime_count:
		Return number of primes in prime cache.
	random_mpint:
		Return random mpint of specified size.
	random_prime_mpint:
		Return random mpint global::DH_KEY_SIZE long.Blocks if cache empty, until
		new primes generated.
	*/
	static void init();
	static unsigned prime_count();
	static mpint random_mpint(const int & bytes);
	static mpint random_prime_mpint();

private:
	number_generator();
	~number_generator();

	//returns the singleton
	static number_generator & Number_Generator();

	/*
	PRNG function that mp_prime_random_ex needs
		buff will be filled with random bytes
		length is how many random bytes desired
		data is an optional pointer to an object that can be passed to PRNG
	*/
	static int PRNG(unsigned char * buff, int length, void * data);

	/*
	All of these functions are associated with public static member functions.
	Look at documentation for static member functions to find out what these do.

	Do not request more than global::DH_KEY_SIZE + 1 from random_mpint_priv().
	*/
	unsigned prime_count_priv();
	mpint random_mpint_priv(const int & bytes);
	mpint random_prime_mpint_priv();

	/*
	Generating primes takes a long time, all this function does is generate
	primes in it's own thread.
	*/
	boost::thread genprime_thread;
	void genprime_loop();

	database::table::prime DB_Prime;
};
#endif
