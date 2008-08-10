#ifndef H_MERSENNE_TWISTER
#define H_MERSENNE_TWISTER

//boost
#include <boost/cstdint.hpp>

class mersenne_twister
{
public:
	mersenne_twister(const int & seed);

	/*
	extract_number   - return a random number
	*/
	boost::uint32_t extract_number();

private:
	int index; //location of last uint32_t used
	boost::uint32_t MT[624];

	/*
	generate_numbers - generates new numbers when all 624 used up
	*/
	void generate_numbers();
};
#endif
