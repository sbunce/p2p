#include "mersenne_twister.h"

mersenne_twister::mersenne_twister():
	index(0),
	seeded(false)
{

}

std::string mersenne_twister::extract_4_bytes()
{
	MT_int tmp;
	tmp.num = extract_number();
	return std::string((char *)tmp.byte, 4);
}

boost::uint32_t mersenne_twister::extract_number()
{
	assert(seeded);
	if(index == 0){
		generate_numbers();
	}
	boost::uint32_t y = MT[index].num;
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
		boost::uint32_t y = ((MT[x].num << 31) >> 31) + ((MT[(x+1) % 624].num >> 1) << 1);
		MT[x].num = MT[(x+397) % 624].num ^ (y >> 1);
		if(y % 2 == 1){
			MT[x].num = MT[x].num ^ 2567483615u;
		}
	}
}

void mersenne_twister::seed(const int & seed)
{
	MT[0].num = seed;
	for(int x=1; x<624; ++x){
		MT[x].num = 1812433253 * (MT[x-1].num ^ (MT[x-1].num >> 30)) + x;
	}
	twist();

	seeded = true;
}

void mersenne_twister::seed(std::string seed)
{
	assert(seed.size() >= 4);

	//make sure seed isn't larger than MT array
	if(seed.size() > 4*624){
		seed.erase(624);
	}

	//fill as much of the array as possible with the seed
	int x;
	for(x=0; x<seed.size(); ++x){
		if(x*4 > seed.size()){
			//if cannot fill all bytes in last location do not fill any
			--x;
			break;
		}

		for(int y=0; y<4; ++y){
			MT[x].byte[y] = seed[x*4+y];
		}
	}

	for(; x<624; ++x){
		MT[x].num = 1812433253 * (MT[x-1].num ^ (MT[x-1].num >> 30)) + x;
	}
	twist();

	seeded = true;
}

void mersenne_twister::twist()
{
	for(int x=0; x<16; ++x){
		generate_numbers();
	}
}
