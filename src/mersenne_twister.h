#ifndef H_MERSENNE_TWISTER
#define H_MERSENNE_TWISTER

//boost
#include <boost/cstdint.hpp>

//std
#include <cassert>
#include <iostream>
#include <string>

class mersenne_twister
{
public:
	mersenne_twister();

	/*
	extract_4_bytes  - returns 4 bytes in a std::string
	extract_number   - return a random number
	seed             - various ways to seed the mersenne twister
	                   one of the seed methods must be done before extract_number()
	                   WARNING: seed value might be modified
	twist            - repeatedly runs generate after seeding
	*/
	std::string extract_4_bytes();
	boost::uint32_t extract_number();
	void seed(const int & seed);
	void seed(std::string seed);
	void twist();

private:
	int index; //location of last uint32_t used

	union MT_int{
		boost::uint32_t num;
		unsigned char byte[sizeof(boost::uint32_t)];
	};

	MT_int MT[624];

	//program will be terminated if mersenne twister not seeded
	bool seeded;

	/*
	generate_numbers - generates new numbers when all 624 used up
	*/
	void generate_numbers();
};
#endif
