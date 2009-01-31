//custom
#include "../atomic_int.hpp"
#include "../global.hpp"

//std
#include <limits>

//assignment (=)
void assignment()
{
	atomic_int<int> x, y;
	int z;

	x = 0;
	y = 1;
	x = y;
	if(x != 1){
		LOGGER; exit(1);
	}

	x = 0;
	z = 1;
	x = z;
	if(x != 1){
		LOGGER; exit(1);
	}
}

//arithmetic (+, -, *, /, %)
void arithmetic()
{
	atomic_int<int> x, y;
	int z;

	//+
	x = 2; y = 2;
	x = x + y;
	if(x != 4){
		LOGGER; exit(1);
	}
	x = 2; z = 2;
	x = x + z;
	if(x != 4){
		LOGGER; exit(1);
	}

	//-
	x = 2; y = 2;
	x = x - y;
	if(x != 0){
		LOGGER; exit(1);
	}
	x = 2; z = 2;
	x = x - z;
	if(x != 0){
		LOGGER; exit(1);
	}

	//*
	x = 2; y = 2;
	x = x * y;
	if(x != 4){
		LOGGER; exit(1);
	}
	x = 2; z = 2;
	x = x * z;
	if(x != 4){
		LOGGER; exit(1);
	}

	///
	x = 2; y = 2;
	x = x / y;
	if(x != 1){
		LOGGER; exit(1);
	}
	x = 2; z = 2;
	x = x / z;
	if(x != 1){
		LOGGER; exit(1);
	}

	//%
	x = 2; y = 2;
	x = x % y;
	if(x != 0){
		LOGGER; exit(1);
	}
	x = 2; z = 2;
	x = x % z;
	if(x != 0){
		LOGGER; exit(1);
	}
}

//increment/decrement (++, --)
void increment_decrement()
{
	atomic_int<int> x;

	//++
	x = 0;
	if(++x != 1){
		LOGGER; exit(1);
	}
	x = 0;
	if(x++ != 0){
		LOGGER; exit(1);
	}

	//--
	x = 1;
	if(--x != 0){
		LOGGER; exit(1);
	}
	x = 1;
	if(x-- != 1){
		LOGGER; exit(1);
	}
}

//relational (==, !=, >, <, >=, <=)
void relational()
{
	atomic_int<int> x, y;
	int z;

	//==
	x = 0, y = 1;
	if(x == y){
		LOGGER; exit(1);
	}
	z = 1;
	if(x == z){
		LOGGER; exit(1);
	}

	//!=
	x = 0, y = 0;
	if(x != y){
		LOGGER; exit(1);
	}
	z = 0;
	if(x != z){
		LOGGER; exit(1);
	}

	//>
	x = 0; y = 1;
	if(x > y){
		LOGGER; exit(1);
	}
	z = 1;
	if(x > z){
		LOGGER; exit(1);
	}

	//<
	x = 1; y = 0;
	if(x < y){
		LOGGER; exit(1);
	}
	z = 0;
	if(x < z){
		LOGGER; exit(1);
	}

	//>=
	x = 0; y = 1;
	if(x >= y){
		LOGGER; exit(1);
	}
	z = 1;
	if(x >= z){
		LOGGER; exit(1);
	}

	//<=
	x = 1; y = 0;
	if(x < y){
		LOGGER; exit(1);
	}
	z = 0;
	if(x < z){
		LOGGER; exit(1);
	}
}

//logical (!, &&, ||)
void logical()
{
	atomic_int<int> x, y;
	int z;

	//!
	x = 1;
	if(!x){
		LOGGER; exit(1);
	}

	//&&
	x = 0; y = 1;
	if(x && y){
		LOGGER; exit(1);
	}
	z = 1;
	if(x && z){
		LOGGER; exit(1);
	}

	//||
	x = 0; y = 0;
	if(x || y){
		LOGGER; exit(1);
	}
	z = 0;
	if(x || z){
		LOGGER; exit(1);
	}
}

//conditional (?)
void conditional()
{
	atomic_int<int> x, y, z;

	//?
	x = 1; y = 2; z = 3;
	x = x ? y : z;
	if(x != 2){
		LOGGER; exit(1);
	}

	//()
	x = 0;
	if(x){
		LOGGER; exit(1);	
	}
}

//bitwise (&, |, ^, ~, <<, >>)
void bitwise()
{
	atomic_int<int> x, y;
	int z;

	//&
	x = 1; y = 3;
	x = x & y;
	if(x != 1){
		LOGGER; exit(1);
	}
	x = 1; z = 3;
	x = x & z;
	if(x != 1){
		LOGGER; exit(1);
	}

	//|
	x = 1; y = 2;
	x = x | y;
	if(x != 3){
		LOGGER; exit(1);
	}
	x = 1; z = 2;
	x = x | z;
	if(x != 3){
		LOGGER; exit(1);
	}

	//^
	x = 1; y = 3;
	x = x ^ y;
	if(x != 2){
		LOGGER; exit(1);
	}
	x = 1; z = 3;
	x = x ^ z;
	if(x != 2){
		LOGGER; exit(1);
	}

	//~
	atomic_int<unsigned> q(std::numeric_limits<unsigned>::max());
	q = ~q;
	if(q != 0u){
		LOGGER; exit(1);
	}

	//<<
	x = 1;
	x = x << 1;
	if(x != 2){
		LOGGER; exit(1);
	}

	//>>
	x = 2;
	x = x >> 1;
	if(x != 1){
		LOGGER; exit(1);
	}
}

//compound assignment (+=, -=, *=, /=, %=, >>=, <<=, &=, ^=, |=)
void compound_assignment()
{
	atomic_int<int> x;
	int y;

	//+=
	x = 1;
	x += x;
	if(x != 2){
		LOGGER; exit(1);
	}
	x = 1; y = 1;
	x += y;
	if(x != 2){
		LOGGER; exit(1);
	}

	//-=
	x = 1;
	x -= x;
	if(x != 0){
		LOGGER; exit(1);
	}
	x = 1; y = 1;
	x -= y;
	if(x != 0){
		LOGGER; exit(1);
	}

	//*=
	x = 1;
	x *= x;
	if(x != 1){
		LOGGER; exit(1);
	}
	x = 1; y = 1;
	x *= y;
	if(x != 1){
		LOGGER; exit(1);
	}

	///=
	x = 2;
	x /= x;
	if(x != 1){
		LOGGER; exit(1);
	}
	x = 2; y = 2;
	x /= y;
	if(x != 1){
		LOGGER; exit(1);
	}

	//%=
	x = 2;
	x %= x;
	if(x != 0){
		LOGGER; exit(1);
	}
	x = 2; y = 2;
	x %= y;
	if(x != 0){
		LOGGER; exit(1);
	}

	//>>=
	x = 2;
	x >>= 1;
	if(x != 1){
		LOGGER; exit(1);
	}

	//<<=
	x = 1;
	x <<= 1;
	if(x != 2){
		LOGGER; exit(1);
	}

	//&=
	atomic_int<unsigned> q(std::numeric_limits<unsigned>::max()), p(0u);
	q &= p;
	if(q != 0u){
		LOGGER; exit(1);
	}
	q = std::numeric_limits<unsigned>::max();;
	q &= 0u;
	if(q != 0u){
		LOGGER; exit(1);
	}

	//^=
	x = 1;
	x ^= x;
	if(x != 0){
		LOGGER; exit(1);
	}
	x = 1;
	x ^= 1;
	if(x != 0){
		LOGGER; exit(1);
	}

	//|=
	x = 1;
	x |= x;
	if(x != 1){
		LOGGER; exit(1);
	}
	x = 1;
	x |= 1;
	if(x != 1){
		LOGGER; exit(1);
	}
}

void stream()
{
	atomic_int<int> x(1);
	std::stringstream ss;
	ss << x;
	if(ss.str() != "1"){
		LOGGER; exit(1);
	}

	ss >> x;
	if(x != 1){
		LOGGER; exit(1);
	}
}

int main()
{
	//note: a lot of these tests are interdependent
	assignment();
	arithmetic();
	increment_decrement();
	relational();
	logical();
	conditional();
	bitwise();
	compound_assignment();
	stream();

	return 0;
}
