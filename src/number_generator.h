#ifndef H_NUMBER_GENERATOR
#define H_NUMBER_GENERATOR

//boost
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

//custom
#include "DB_prime.h"
#include "global.h"

//libtommath
#include <mpint.h>

//std
#include <fstream>
#include <iostream>
#include <vector>

class number_generator
{
public:

	/*
	Returns the amount of primes that are waiting in cache to be returned by the
	random_prime_mpint() function.
	*/
	static unsigned int prime_count()
	{
		init();
		return ((number_generator *)Number_Generator)->prime_count_priv();
	}

	/*
	Returns a random mpint that is bytes long.
	*/
	static mpint random_mpint(const int & bytes)
	{
		init();
		return ((number_generator *)Number_Generator)->random_mpint_priv(bytes);
	}

	/*
	Returns a random prime mpint that is 128 bytes long. This is used for the
	Diffie_Hellman in encryption class.
	*/
	static mpint random_prime_mpint()
	{
		init();
		return ((number_generator *)Number_Generator)->random_prime_mpint_priv();
	}

	//function to initialize the singleton
	inline static void init()
	{
		boost::mutex::scoped_lock lock(Mutex);
		if(Number_Generator == NULL){
			Number_Generator = new number_generator;
		}
	}

private:
	//only the number generator can initialize itself
	number_generator();

	static number_generator * Number_Generator; //the one possible instance
	static boost::mutex Mutex;                  //mutex for checking if singleton was initialized

	//PRNG function that mp_prime_random_ex needs
	static int PRNG(unsigned char * buff, int length, void * data)
	{
		#ifdef WIN32
		std::cout << "NEED TO ADD WINDOWS PRNG\n";
		exit(1);
		#else
		std::fstream fin("/dev/urandom", std::ios::in | std::ios::binary);
		char ch;
		int n = length;
		while(length--){
			fin.get(ch);
			*buff++ = (unsigned char)ch;
		}
		#endif
		return n; //return how many random bytes added to buff
	}

	/*
	All of these functions are associated with public static member functions.
	Look at documentation for static member functions to find out what these do.
	*/
	unsigned int prime_count_priv();
	mpint random_mpint_priv(const int & bytes);
	mpint random_prime_mpint_priv();

	/*
	Generating primes takes a long time, all this function does is generate
	primes in it's own thread.
	*/
	void generate_primes_thread();

	DB_prime DB_Prime;
};
#endif
