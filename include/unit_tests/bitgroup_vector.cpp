//include
#include <bitgroup_vector.hpp>
#include <timer.hpp>

//standard
#include <cmath>

void assignment()
{
	unsigned groups = 1024;
	//try all different group sizes
	for(unsigned x=1; x<8; ++x){
		bitgroup_vector BGV(groups, x);
		unsigned max = std::pow(static_cast<const double>(2), static_cast<const int>(x)) - 1;
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

void named_functions()
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

void operators()
{
	bitgroup_vector BGV_0(2, 1);
	bitgroup_vector BGV_1(2, 1);

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

	//bitgroup_vector::proxy assignment test
	BGV_0[0] = BGV_0[1] = 1;
	if(BGV_0[0] != 1 || BGV_0[1] != 1){
		LOGGER; exit(1);
	}
}

int main()
{
	assignment();
	named_functions();
	operators();
}
