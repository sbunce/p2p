//include
#include <bitgroup_set.hpp>
#include <timer.hpp>

//standard
#include <cmath>
#include <limits>

//does exhaustive test of different assignment operations
void test_assignment()
{
	unsigned groups = 1024;
	//try all different group sizes
	for(unsigned x=1; x<8; ++x){
		bitgroup_set BGS(groups, x);
		unsigned max = std::pow(2, x) - 1;
		unsigned temp = 0;
		//set all bitgroups to known values
		for(unsigned y=0; y<groups; ++y){
			BGS[y] = temp++ % max;
		}
		temp = 0;
		//test for known values to make sure read/write works
		for(unsigned y=0; y<groups; ++y){
			assert(BGS[y] == temp++ % max);
		} 
	}
}

void test_named_functions()
{
	bitgroup_set BGS(1, 1);

	//size function
	if(BGS.size() != 1){
		LOGGER; exit(1);
	}

	//reset function
	BGS[0] = 1;
	if(BGS[0] != 1){
		LOGGER; exit(1);
	}
	BGS.reset();
	if(BGS[0] == 1){
		LOGGER; exit(1);
	}
}

void test_operators()
{
	bitgroup_set BGS_0(1, 1);
	bitgroup_set BGS_1(1, 1);

	//==
	if(!(BGS_0 == BGS_1)){
		LOGGER; exit(1);
	}

	//!=
	if(BGS_0 != BGS_1){
		LOGGER; exit(1);
	}

	//&=
	BGS_0 &= BGS_1;
	if(BGS_0[0] != 0){
		LOGGER; exit(1);
	}

	//^=
	BGS_0 ^= BGS_1;
	if(BGS_0[0] != 0){
		LOGGER; exit(1);
	}

	//|=
	BGS_0 |= BGS_1;
	if(BGS_0[0] != 0){
		LOGGER; exit(1);
	}

	//~
	~BGS_0;
	if(BGS_0[0] != 1){
		LOGGER; exit(1);
	}

	//=
	BGS_0 = BGS_1;
	if(BGS_0[0] != 0){
		LOGGER; exit(1);
	}
}

int main()
{
	test_assignment();
	test_named_functions();
	test_operators();
}
