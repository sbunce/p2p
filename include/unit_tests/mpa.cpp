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
		LOGGER; ++fail;
	}
	}

	//ctor for string and radix
	{
	mpint x("2");
	if(x != "2"){
		LOGGER; ++fail;
	}
	mpint y("A", 16);
	if(y != "10"){
		LOGGER; ++fail;
	}
	}

	//ctor for binary bytes
	{
	mpint x("2");
	std::string bin = x.bin();
	mpint y(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	if(y != "2"){
		LOGGER; ++fail;
	}
	}

	//sqr
	{
	mpint a = "2";
	a = sqr(a);
	if(a != "4"){
		LOGGER; ++fail;
	}
	}

	//sqrt
	{
	mpint x = "4";
	x = sqrt(x);
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//n_root
	{
	mpint x = "4";
	x = n_root(x, 2);
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//mul_2
	{
	mpint x = "2";
	x = mul_2(x);
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//div_2
	{
	mpint x = "2";
	x = div_2(x);
	if(x != "1"){
		LOGGER; ++fail;
	}
	}

	//mul_2d
	{
	mpint x = "2";
	x = mul_2d(x, 2);
	if(x != "8"){
		LOGGER; ++fail;
	}
	}

	//div_2d
	{
	mpint a = "2", c, d;
	div_2d(a, 1, c, d);
	if(c != "1"){
		LOGGER; ++fail;
	}
	if(d != "0"){
		LOGGER; ++fail;
	}
	}

	//mod_2d
	{
	mpint a = "2";
	a = mod_2d(a, 1);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//addmod
	{
	mpint a = "1", b = "1", c = "2";
	a = addmod(a, b, c);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//submod
	{
	mpint a = "3", b = "1", c = "2";
	a = submod(a, b, c);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//mulmod
	{
	mpint a = "2", b = "1", c = "2";
	a = mulmod(a, b, c);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//sqrmod
	{
	mpint a = "2", b = "2";
	a = sqrmod(a, b);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//invmod
	{
	mpint a = "1", b = "1";
	a = sqrmod(a, b);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//exptmod
	{
	mpint a = "4", b = "1", c = "2";
	a = exptmod(a, b, c);
	if(a != "0"){
		LOGGER; ++fail;
	}
	}

	//is_prime
	{
	mpint x = "4";
	if(is_prime(x)){
		LOGGER; ++fail;
	}
	}

	//next_prime
	{
	mpint x = "1";
	x = next_prime(x);
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//gcd
	{
	mpint x = "4", y = "8";
	x = gcd(x, y);
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//lcm
	{
	mpint x = "4", y = "8";
	x = lcm(x, y);
	if(x != "8"){
		LOGGER; ++fail;
	}
	}

	//random_prime
	{
	mpint x = mpa::random_prime(16);
	if(!mpa::is_prime(x)){
		LOGGER; ++fail;
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
		LOGGER; ++fail;
	}
	}

	//operator +
	{
	mpint x = "2", y = "2";
	x = x+y;
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//operator -
	{
	mpint x = "4", y = "2";
	x = x-y;
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//operator *
	{
	mpint x = "2";
	x = x*x;
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//operator /
	{
	mpint x = "2";
	x = x/x;
	if(x != "1"){
		LOGGER; ++fail;
	}
	}

	//operator &
	{
	mpint x = "1", y = "2";
	x = x&x;
	if(x != "1"){
		LOGGER; ++fail;
	}
	}

	//operator |
	{
	mpint x = "1", y = "2";
	x = x|y;
	if(x != "3"){
		LOGGER; ++fail;
	}
	}

	//operator ^
	{
	mpint x = "1", y = "3";
	x = x^y;
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//operator %
	{
	mpint x = "2", y = "2";
	x = x%y;
	if(x != "0"){
		LOGGER; ++fail;
	}
	}

	//operator -
	{
	mpint x = "1";
	x = -x;
	if(x != "-1"){
		LOGGER; ++fail;
	}
	}

	//operator +=
	{
	mpint x = "2";
	x += x;
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//operator -=
	{
	mpint x = "2";
	x -= x;
	if(x != "0"){
		LOGGER; ++fail;
	}
	}

	//operator *=
	{
	mpint x = "2";
	x *= x;
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//operator /=
	{
	mpint x = "2";
	x /= x;
	if(x != "1"){
		LOGGER; ++fail;
	}
	}

	//operator &=
	{
	mpint x = "2";
	x &= x;
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//operator |=
	{
	mpint x = "2";
	x |= x;
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//operator ^=
	{
	mpint x = "2";
	x ^= x;
	if(x != "0"){
		LOGGER; ++fail;
	}
	}

	//operator %=
	{
	mpint x = "2";
	x %= x;
	if(x != "0"){
		LOGGER; ++fail;
	}
	}

	//operator <<=
	{
	mpint x = "2";
	x <<= 1;
	if(x != "4"){
		LOGGER; ++fail;
	}
	}

	//operator >>=
	{
	mpint x = "2";
	x >>= 1;
	if(x != "1"){
		LOGGER; ++fail;
	}
	}

	//operator ++ (pre)
	{
	mpint x = "1";
	if(++x != "2"){
		LOGGER; ++fail;
	}
	}

	//operator -- (pre)
	{
	mpint x = "1";
	if(--x != "0"){
		LOGGER; ++fail;
	}
	}

	//operator ++ (post)
	{
	mpint x = "1";
	if(x++ != "1"){
		LOGGER; ++fail;
	}
	}

	//operator -- (post)
	{
	mpint x = "1";
	if(x-- != "1"){
		LOGGER; ++fail;
	}
	}

	//operator ==
	{
	mpint x = "1", y = "2";
	if(x == y){
		LOGGER; ++fail;
	}
	}

	//operator !=
	{
	mpint x = "1", y = "1";
	if(x != y){
		LOGGER; ++fail;
	}
	}

	//operator >
	{
	mpint x = "1", y = "2";
	if(x > y){
		LOGGER; ++fail;
	}
	}

	//operator <
	{
	mpint x = "2", y = "1";
	if(x < y){
		LOGGER; ++fail;
	}
	}

	//operator >=
	{
	mpint x = "1", y = "2";
	if(x >= y){
		LOGGER; ++fail;
	}
	}

	//operator <=
	{
	mpint x = "2", y = "1";
	if(x <= y){
		LOGGER; ++fail;
	}
	}

	//operator <<
	{
	mpint x = "2";
	if(x << 1 != "4"){
		LOGGER; ++fail;
	}
	}

	//operator >>
	{
	mpint x = "2";
	if(x >> 1 != "1"){
		LOGGER; ++fail;
	}
	}

	return fail;
}
