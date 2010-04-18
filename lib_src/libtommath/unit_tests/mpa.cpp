//include
#include <logger.hpp>
#include <mpa.hpp>

int fail(0);

using namespace mpa;

int main()
{
	//default ctor
	{
	mpint x("2");
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//ctor for string and radix
	{
	mpint x("2");
	if(x != "2"){
		LOG; ++fail;
	}
	mpint y("A", 16);
	if(y != "10"){
		LOG; ++fail;
	}
	}

	//ctor for binary bytes
	{
	mpint x("2");
	std::string bin = x.bin();
	mpint y(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	if(y != "2"){
		LOG; ++fail;
	}
	}

	//sqr
	{
	mpint a = "2";
	a = sqr(a);
	if(a != "4"){
		LOG; ++fail;
	}
	}

	//sqrt
	{
	mpint x = "4";
	x = sqrt(x);
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//n_root
	{
	mpint x = "4";
	x = n_root(x, 2);
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//mul_2
	{
	mpint x = "2";
	x = mul_2(x);
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//div_2
	{
	mpint x = "2";
	x = div_2(x);
	if(x != "1"){
		LOG; ++fail;
	}
	}

	//mul_2d
	{
	mpint x = "2";
	x = mul_2d(x, 2);
	if(x != "8"){
		LOG; ++fail;
	}
	}

	//div_2d
	{
	mpint a = "2", c, d;
	div_2d(a, 1, c, d);
	if(c != "1"){
		LOG; ++fail;
	}
	if(d != "0"){
		LOG; ++fail;
	}
	}

	//mod_2d
	{
	mpint a = "2";
	a = mod_2d(a, 1);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//addmod
	{
	mpint a = "1", b = "1", c = "2";
	a = addmod(a, b, c);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//submod
	{
	mpint a = "3", b = "1", c = "2";
	a = submod(a, b, c);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//mulmod
	{
	mpint a = "2", b = "1", c = "2";
	a = mulmod(a, b, c);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//sqrmod
	{
	mpint a = "2", b = "2";
	a = sqrmod(a, b);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//invmod
	{
	mpint a = "1", b = "1";
	a = sqrmod(a, b);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//exptmod
	{
	mpint a = "4", b = "1", c = "2";
	a = exptmod(a, b, c);
	if(a != "0"){
		LOG; ++fail;
	}
	}

	//is_prime
	{
	mpint x = "4";
	if(is_prime(x)){
		LOG; ++fail;
	}
	}

	//next_prime
	{
	mpint x = "1";
	x = next_prime(x);
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//gcd
	{
	mpint x = "4", y = "8";
	x = gcd(x, y);
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//lcm
	{
	mpint x = "4", y = "8";
	x = lcm(x, y);
	if(x != "8"){
		LOG; ++fail;
	}
	}

	//random_prime
	{
	mpint x = mpa::random_prime(16);
	if(!mpa::is_prime(x)){
		LOG; ++fail;
	}
	}

	//random
	{
	mpint x = mpa::random(16);
	}

	//operator =
	{
	mpint x = "1", y = "2";
	x = y;
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//operator +
	{
	mpint x = "2", y = "2";
	x = x+y;
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//operator -
	{
	mpint x = "4", y = "2";
	x = x-y;
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//operator *
	{
	mpint x = "2";
	x = x*x;
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//operator /
	{
	mpint x = "2";
	x = x/x;
	if(x != "1"){
		LOG; ++fail;
	}
	}

	//operator &
	{
	mpint x = "1", y = "2";
	x = x&x;
	if(x != "1"){
		LOG; ++fail;
	}
	}

	//operator |
	{
	mpint x = "1", y = "2";
	x = x|y;
	if(x != "3"){
		LOG; ++fail;
	}
	}

	//operator ^
	{
	mpint x = "1", y = "3";
	x = x^y;
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//operator %
	{
	mpint x = "2", y = "2";
	x = x%y;
	if(x != "0"){
		LOG; ++fail;
	}
	}

	//operator -
	{
	mpint x = "1";
	x = -x;
	if(x != "-1"){
		LOG; ++fail;
	}
	}

	//operator +=
	{
	mpint x = "2";
	x += x;
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//operator -=
	{
	mpint x = "2";
	x -= x;
	if(x != "0"){
		LOG; ++fail;
	}
	}

	//operator *=
	{
	mpint x = "2";
	x *= x;
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//operator /=
	{
	mpint x = "2";
	x /= x;
	if(x != "1"){
		LOG; ++fail;
	}
	}

	//operator &=
	{
	mpint x = "2";
	x &= x;
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//operator |=
	{
	mpint x = "2";
	x |= x;
	if(x != "2"){
		LOG; ++fail;
	}
	}

	//operator ^=
	{
	mpint x = "2";
	x ^= x;
	if(x != "0"){
		LOG; ++fail;
	}
	}

	//operator %=
	{
	mpint x = "2";
	x %= x;
	if(x != "0"){
		LOG; ++fail;
	}
	}

	//operator <<=
	{
	mpint x = "2";
	x <<= 1;
	if(x != "4"){
		LOG; ++fail;
	}
	}

	//operator >>=
	{
	mpint x = "2";
	x >>= 1;
	if(x != "1"){
		LOG; ++fail;
	}
	}

	//operator ++ (pre)
	{
	mpint x = "1";
	if(++x != "2"){
		LOG; ++fail;
	}
	}

	//operator -- (pre)
	{
	mpint x = "1";
	if(--x != "0"){
		LOG; ++fail;
	}
	}

	//operator ++ (post)
	{
	mpint x = "1";
	if(x++ != "1"){
		LOG; ++fail;
	}
	}

	//operator -- (post)
	{
	mpint x = "1";
	if(x-- != "1"){
		LOG; ++fail;
	}
	}

	//operator ==
	{
	mpint x = "1", y = "2";
	if(x == y){
		LOG; ++fail;
	}
	}

	//operator !=
	{
	mpint x = "1", y = "1";
	if(x != y){
		LOG; ++fail;
	}
	}

	//operator >
	{
	mpint x = "1", y = "2";
	if(x > y){
		LOG; ++fail;
	}
	}

	//operator <
	{
	mpint x = "2", y = "1";
	if(x < y){
		LOG; ++fail;
	}
	}

	//operator >=
	{
	mpint x = "1", y = "2";
	if(x >= y){
		LOG; ++fail;
	}
	}

	//operator <=
	{
	mpint x = "2", y = "1";
	if(x <= y){
		LOG; ++fail;
	}
	}

	//operator <<
	{
	mpint x = "2";
	if(x << 1 != "4"){
		LOG; ++fail;
	}
	}

	//operator >>
	{
	mpint x = "2";
	if(x >> 1 != "1"){
		LOG; ++fail;
	}
	}

	return fail;
}
