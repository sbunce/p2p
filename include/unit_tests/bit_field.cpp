//include
#include <bit_field.hpp>
#include <logger.hpp>
#include <unit_test.hpp>

//standard
#include <cmath>

int fail(0);

/*
Compare bits set to 1 according to bit_field::set_count() and bits set to 1 by
using the [] operator to count. The bit_field keeps an internal counter to know
how many bits are set to 1. This test verifies the state of that counter.
*/
void check_count(const bit_field & BF)
{
	boost::uint64_t count = 0;
	for(boost::uint64_t x = 0; x<BF.size(); ++x){
		count += BF[x];
	}
	if(count != BF.set_count()){
		LOG; ++fail;
	}
}

void assignment()
{
	//starting with every bit off, turn every even bit on
	boost::uint64_t test_size = 16;
	bit_field BF(test_size);
	for(boost::uint64_t x=0; x<test_size; ++x){
		if(x % 2 == 0){
			BF[x] = true;
		}
	}
	check_count(BF);

	//make sure every even bit on
	for(boost::uint64_t x=0; x<test_size; ++x){
		if(x % 2 == 0){
			if(BF[x] != true){
				LOG; ++fail;
			}
		}
	}
	check_count(BF);

	//starting with every bit on, turn every even bit off
	BF.set();
	for(boost::uint64_t x=0; x<test_size; ++x){
		if(x % 2 == 0){
			BF[x] = false;
		}
	}
	check_count(BF);

	//make sure every even bit off
	for(boost::uint64_t x=0; x<test_size; ++x){
		if(x % 2 == 0){
			if(BF[x] != false){
				LOG; ++fail;
			}
		}
	}
	check_count(BF);
}

void named_functions()
{
	bit_field BF(1);

	//size function
	if(BF.size() != 1){
		LOG; ++fail;
	}

	//reset function
	BF[0] = 1;
	if(BF[0] != 1){
		LOG; ++fail;
	}
	check_count(BF);
	BF.reset();
	if(BF[0] == 1){
		LOG; ++fail;
	}
	check_count(BF);
}

void operators()
{
	bit_field BF_0(2);
	bit_field BF_1(2);
	check_count(BF_0);

	//==
	if(!(BF_0 == BF_1)){
		LOG; ++fail;
	}

	//!=
	if(BF_0 != BF_1){
		LOG; ++fail;
	}

	//&=
	BF_0 &= BF_1;
	if(BF_0[0] != 0){
		LOG; ++fail;
	}
	check_count(BF_0);

	//^=
	BF_0 ^= BF_1;
	if(BF_0[0] != 0){
		LOG; ++fail;
	}
	check_count(BF_0);

	//|=
	BF_0 |= BF_1;
	if(BF_0[0] != 0){
		LOG; ++fail;
	}
	check_count(BF_0);

	//~
	~BF_0;
	if(BF_0[0] != 1){
		LOG; ++fail;
	}
	check_count(BF_0);

	//=
	BF_0 = BF_1;
	if(BF_0[0] != 0){
		LOG; ++fail;
	}
	check_count(BF_0);

	//bitgroup_vector::proxy assignment test
	BF_0[0] = BF_0[1] = 1;
	if(BF_0[0] != 1 || BF_0[1] != 1){
		LOG; ++fail;
	}
	check_count(BF_0);
}

int main()
{
	unit_test::timeout();
	assignment();
	named_functions();
	operators();
	return fail;
}
