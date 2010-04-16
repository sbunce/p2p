//include
#include <atomic_int.hpp>

//standard
#include <limits>

int fail(0);

void assignment()
{
	atomic_int<int> x, y;
	int z;

	x = 0;
	y = 1;
	x = y;
	if(x != 1){
		LOG; ++fail;
	}

	x = 0;
	z = 1;
	x = z;
	if(x != 1){
		LOG; ++fail;
	}
}

void emulate()
{
	atomic_int<int> x, y, z;

	//?
	x = 1; y = 2; z = 3;
	x = x ? y : z;
	if(x != 2){
		LOG; ++fail;
	}

	//()
	x = 0;
	if(x){
		LOG; ++fail;	
	}
}

void increment_decrement()
{
	atomic_int<int> x;

	//++
	x = 0;
	if(++x != 1){
		LOG; ++fail;
	}
	x = 0;
	if(x++ != 0){
		LOG; ++fail;
	}

	//--
	x = 1;
	if(--x != 0){
		LOG; ++fail;
	}
	x = 1;
	if(x-- != 1){
		LOG; ++fail;
	}
}


void compound_assignment()
{
	atomic_int<int> x;
	int y;

	//+=
	x = 1;
	x += x;
	if(x != 2){
		LOG; ++fail;
	}
	x = 1; y = 1;
	x += y;
	if(x != 2){
		LOG; ++fail;
	}

	//-=
	x = 1;
	x -= x;
	if(x != 0){
		LOG; ++fail;
	}
	x = 1; y = 1;
	x -= y;
	if(x != 0){
		LOG; ++fail;
	}

	//*=
	x = 1;
	x *= x;
	if(x != 1){
		LOG; ++fail;
	}
	x = 1; y = 1;
	x *= y;
	if(x != 1){
		LOG; ++fail;
	}

	///=
	x = 2;
	x /= x;
	if(x != 1){
		LOG; ++fail;
	}
	x = 2; y = 2;
	x /= y;
	if(x != 1){
		LOG; ++fail;
	}

	//%=
	x = 2;
	x %= x;
	if(x != 0){
		LOG; ++fail;
	}
	x = 2; y = 2;
	x %= y;
	if(x != 0){
		LOG; ++fail;
	}

	//>>=
	x = 2;
	x >>= 1;
	if(x != 1){
		LOG; ++fail;
	}

	//<<=
	x = 1;
	x <<= 1;
	if(x != 2){
		LOG; ++fail;
	}

	//&=
	atomic_int<unsigned> q(std::numeric_limits<unsigned>::max()), p(0u);
	q &= p;
	if(q != 0u){
		LOG; ++fail;
	}
	q = std::numeric_limits<unsigned>::max();;
	q &= 0u;
	if(q != 0u){
		LOG; ++fail;
	}

	//^=
	x = 1;
	x ^= x;
	if(x != 0){
		LOG; ++fail;
	}
	x = 1;
	x ^= 1;
	if(x != 0){
		LOG; ++fail;
	}

	//|=
	x = 1;
	x |= x;
	if(x != 1){
		LOG; ++fail;
	}
	x = 1;
	x |= 1;
	if(x != 1){
		LOG; ++fail;
	}
}

void stream()
{
	atomic_int<int> x(1);
	std::stringstream ss;
	ss << x;
	if(ss.str() != "1"){
		LOG; ++fail;
	}

	ss >> x;
	if(x != 1){
		LOG; ++fail;
	}
}

int main()
{
	assignment();
	emulate();
	increment_decrement();
	compound_assignment();
	stream();
	return fail;
}
