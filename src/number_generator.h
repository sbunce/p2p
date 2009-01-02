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

#ifdef WIN32
#include <wincrypt.h>
#endif

class number_generator
{
public:

	/*
	Returns the amount of primes that are waiting in cache to be returned by the
	random_prime_mpint() function.
	*/
	static unsigned prime_count()
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

	/*
	PRNG function that mp_prime_random_ex needs
		buff will be filled with random bytes
		length is how many random bytes desired
		data is an optional pointer to an object that can be passed to PRNG
	*/
	static int PRNG(unsigned char * buff, int length, void * data)
	{
#ifdef WIN32
		HCRYPTPROV hProvider = 0;
		if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
			if(CryptGenRandom(hProvider, sizeof(buff), buff)){
				CryptReleaseContext(hProvider, 0); 
				return length;
			}
		}
		LOGGER << "error generating random number";
		exit(1);
#else
		char ch;
		int n = length;
		std::fstream fin("/dev/urandom", std::ios::in | std::ios::binary);
		while(length--){
			fin.get(ch);
			*buff++ = (unsigned char)ch;
		}
		return n;
#endif
	}

	/*
	All of these functions are associated with public static member functions.
	Look at documentation for static member functions to find out what these do.

	Do not request more than global::DH_KEY_SIZE + 1 from random_mpint_priv().
	*/
	unsigned int prime_count_priv();
	mpint random_mpint_priv(const int & bytes);
	mpint random_prime_mpint_priv();

	/*
	Generating primes takes a long time, all this function does is generate
	primes in it's own thread.
	*/
	boost::thread genprime_thread;
	void genprime_loop();

	DB_prime DB_Prime;
};
#endif
