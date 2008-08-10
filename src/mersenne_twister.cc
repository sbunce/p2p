#include "mersenne_twister.h"

mersenne_twister::mersenne_twister(
	const int & seed
):
	index(0)
{
	MT[0] = seed;
	for(int x=1; x<624; ++x){
		MT[x] = 1812433253 * (MT[x-1] ^ (MT[x-1] >> 30)) + x;
	}
}

boost::uint32_t mersenne_twister::extract_number()
{
	if(index == 0){
		generate_numbers();
	}
	boost::uint32_t y = MT[index];
	y ^= y >> 11;
	y ^= (y << 7) & 2636928640u;
	y ^= (y << 15) & 4022730752u;
	y ^= y >> 18;
	index = ++index % 624;
	return y;
}

void mersenne_twister::generate_numbers()
{
	for(int x=0; x<624; ++x){
		boost::uint32_t y = ((MT[x] << 31) >> 31) + ((MT[(x+1) % 624] >> 1) << 1);
		MT[x] = MT[(x+397) % 624] ^ (y >> 1);
		if(y % 2 == 1){
			MT[x] = MT[x] ^ 2567483615u;
		}
	}
}
