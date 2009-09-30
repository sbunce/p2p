//include
#include <bit_field.hpp>

//standard
#include <cmath>

void assignment()
{
	unsigned groups = 1024;
	//try all different group sizes
	for(unsigned x=1; x<8; ++x){
		bit_field BF(groups, x);
		unsigned max = std::pow(static_cast<const double>(2), static_cast<const int>(x)) - 1;
		unsigned temp = 0;
		//set all bitgroups to known values
		for(unsigned y=0; y<groups; ++y){
			BF[y] = temp++ % max;
		}
		temp = 0;
		//test for known values to make sure read/write works
		for(unsigned y=0; y<groups; ++y){
			assert(BF[y] == temp++ % max);
		} 
	}
}

void named_functions()
{
	bit_field BF(1, 1);

	//size function
	if(BF.size() != 1){
		LOGGER; exit(1);
	}

	//reset function
	BF[0] = 1;
	if(BF[0] != 1){
		LOGGER; exit(1);
	}
	BF.reset();
	if(BF[0] == 1){
		LOGGER; exit(1);
	}
}

void operators()
{
	bit_field BF_0(2, 1);
	bit_field BF_1(2, 1);

	//==
	if(!(BF_0 == BF_1)){
		LOGGER; exit(1);
	}

	//!=
	if(BF_0 != BF_1){
		LOGGER; exit(1);
	}

	//&=
	BF_0 &= BF_1;
	if(BF_0[0] != 0){
		LOGGER; exit(1);
	}

	//^=
	BF_0 ^= BF_1;
	if(BF_0[0] != 0){
		LOGGER; exit(1);
	}

	//|=
	BF_0 |= BF_1;
	if(BF_0[0] != 0){
		LOGGER; exit(1);
	}

	//~
	~BF_0;
	if(BF_0[0] != 1){
		LOGGER; exit(1);
	}

	//=
	BF_0 = BF_1;
	if(BF_0[0] != 0){
		LOGGER; exit(1);
	}

	//bitgroup_vector::proxy assignment test
	BF_0[0] = BF_0[1] = 1;
	if(BF_0[0] != 1 || BF_0[1] != 1){
		LOGGER; exit(1);
	}
}

int main()
{
	assignment();
	named_functions();
	operators();
}
