//include
#include <atomic_int.hpp>

//standard
#include <limits>
#include <unit_test.hpp>

int fail(0);

void assignment()
{
	{//atomic_int rval
	atomic_int<int> x = 0, y = 1;
	x = y;
	if(x != 1){
		LOG; ++fail;
	}
	}

	{//int rval
	atomic_int<int> x = 0;
	int y = 1;
	x = y;
	if(x != 1){
		LOG; ++fail;
	}
	}
}

void conversion()
{
	{
	atomic_int<int> x = 0;
	if(x){
		LOG; ++fail;	
	}
	}

	{
	atomic_int<int> x = 1;
	if(!x){
		LOG; ++fail;	
	}
	}
}

void increment_decrement()
{
	{//pre-inc
	atomic_int<int> x = 0;
	if(++x != 1){
		LOG; ++fail;
	}
	}

	{//post-inc
	atomic_int<int> x = 0;
	if(x++ != 0){
		LOG; ++fail;
	}
	if(x != 1){
		LOG; ++fail;
	}
	}

	{//pre-dec
	atomic_int<int> x = 1;
	if(--x != 0){
		LOG; ++fail;
	}
	}

	{//post-dec
	atomic_int<int> x = 1;
	if(x-- != 1){
		LOG; ++fail;
	}
	if(x != 0){
		LOG; ++fail;
	}
	}
}


void compound_assignment()
{
	{//+= atomic_int rval
	atomic_int<int> x = 1;
	x += x;
	if(x != 2){
		LOG; ++fail;
	}
	}

	{//+= int rval
	atomic_int<int> x = 1;
	int y = 1;
	x += y;
	if(x != 2){
		LOG; ++fail;
	}
	}

	{//-= atomic_int rval
	atomic_int<int> x = 1;
	x -= x;
	if(x != 0){
		LOG; ++fail;
	}
	}

	{//-= int rval
	atomic_int<int> x = 1;
	int y = 1;
	x -= y;
	if(x != 0){
		LOG; ++fail;
	}
	}

	{//*= atomic_int rval
	atomic_int<int> x = 2;
	x *= x;
	if(x != 4){
		LOG; ++fail;
	}
	}

	{//*= int rval
	atomic_int<int> x = 2;
	int y = 2;
	x *= y;
	if(x != 4){
		LOG; ++fail;
	}
	}

	{///= atomic_int rval
	atomic_int<int> x = 2;
	x /= x;
	if(x != 1){
		LOG; ++fail;
	}
	}

	{///= int rval
	atomic_int<int> x = 2;
	int y = 2;
	x /= y;
	if(x != 1){
		LOG; ++fail;
	}
	}

	{//%= atomic_int rval
	atomic_int<int> x = 2;
	x %= x;
	if(x != 0){
		LOG; ++fail;
	}
	}

	{//%= int rval
	atomic_int<int> x = 2;
	int y = 2;
	x %= y;
	if(x != 0){
		LOG; ++fail;
	}
	}

	{//>>=
	atomic_int<int> x = 2;
	x >>= 1;
	if(x != 1){
		LOG; ++fail;
	}
	}

	{//<<=
	atomic_int<int> x = 1;
	x <<= 1;
	if(x != 2){
		LOG; ++fail;
	}
	}

	{//&= atomic_int rval
	atomic_int<unsigned> x(std::numeric_limits<unsigned>::max()), y(0u);
	x &= y;
	if(x != 0u){
		LOG; ++fail;
	}
	}

	{//&= int rval
	atomic_int<unsigned> q = std::numeric_limits<unsigned>::max();
	q &= 0u;
	if(q != 0u){
		LOG; ++fail;
	}
	}

	{//^= atomic_int rval
	atomic_int<unsigned> x = 1;
	x ^= x;
	if(x != 0){
		LOG; ++fail;
	}
	}

	{//^= int rval
	atomic_int<unsigned> x = 1;
	x ^= 1;
	if(x != 0){
		LOG; ++fail;
	}
	}

	{//|= atomic_int rval
	atomic_int<unsigned> x = 1;
	x |= x;
	if(x != 1){
		LOG; ++fail;
	}
	}

	{//|= int rval
	atomic_int<unsigned> x = 1;
	x |= 1;
	if(x != 1){
		LOG; ++fail;
	}
	}
}

void stream()
{
	//ostream
	atomic_int<int> x = 1;
	std::stringstream ss;
	ss << x;
	if(ss.str() != "1"){
		LOG; ++fail;
	}

	//istream
	ss >> x;
	if(x != 1){
		LOG; ++fail;
	}
}

int main()
{
	unit_test::timeout();
	assignment();
	conversion();
	increment_decrement();
	compound_assignment();
	stream();
	return fail;
}
