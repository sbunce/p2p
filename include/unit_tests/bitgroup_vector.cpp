//include
#include <bitgroup_vector.hpp>
#include <timer.hpp>

//standard
#include <algorithm>
#include <cmath>
#include <limits>

//does exhaustive test of different assignment operations
void test_assignment()
{
	unsigned groups = 1024;
	//try all different group sizes
	for(unsigned x=1; x<8; ++x){
		bitgroup_vector BGV(groups, x);
		unsigned max = std::pow(2, x) - 1;
		unsigned temp = 0;
		//set all bitgroups to known values
		for(unsigned y=0; y<groups; ++y){
			BGV[y] = temp++ % max;
		}
		temp = 0;
		//test for known values to make sure read/write works
		for(unsigned y=0; y<groups; ++y){
			assert(BGV[y] == temp++ % max);
		} 
	}
}

void test_named_functions()
{
	bitgroup_vector BGV(1, 1);

	//size function
	if(BGV.size() != 1){
		LOGGER; exit(1);
	}

	//reset function
	BGV[0] = 1;
	if(BGV[0] != 1){
		LOGGER; exit(1);
	}
	BGV.reset();
	if(BGV[0] == 1){
		LOGGER; exit(1);
	}
}

void test_operators()
{
	bitgroup_vector BGV_0(1, 1);
	bitgroup_vector BGV_1(1, 1);

	//==
	if(!(BGV_0 == BGV_1)){
		LOGGER; exit(1);
	}

	//!=
	if(BGV_0 != BGV_1){
		LOGGER; exit(1);
	}

	//&=
	BGV_0 &= BGV_1;
	if(BGV_0[0] != 0){
		LOGGER; exit(1);
	}

	//^=
	BGV_0 ^= BGV_1;
	if(BGV_0[0] != 0){
		LOGGER; exit(1);
	}

	//|=
	BGV_0 |= BGV_1;
	if(BGV_0[0] != 0){
		LOGGER; exit(1);
	}

	//~
	~BGV_0;
	if(BGV_0[0] != 1){
		LOGGER; exit(1);
	}

	//=
	BGV_0 = BGV_1;
	if(BGV_0[0] != 0){
		LOGGER; exit(1);
	}
}

void test_iterator()
{
	bitgroup_vector BGV(8, 1);

	//set every other bigroup to 0
	for(bitgroup_vector::iterator iter_cur = BGV.begin(), iter_end = BGV.end();
		iter_cur != iter_end; ++iter_cur)
	{
		*iter_cur = 1;
	}
}

int main()
{
	test_assignment();
	test_named_functions();
	test_operators();
	test_iterator();

//infinite loop
	bitgroup_vector BGV(8, 1);
	BGV[0] = BGV[1] = BGV[2] = BGV[3] = 1;
	//LOGGER << BGV[0] << BGV[1] << BGV[2] << BGV[3];
}
