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
		LOGGER(logger::utest); ++fail;
	}
	}

	//ctor for string and radix
	{
	mpint x("2");
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	mpint y("A", 16);
	if(y != "10"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//ctor for binary bytes
	{
	mpint x("2");
	std::string bin = x.bin();
	mpint y(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	if(y != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//sqr
	{
	mpint a = "2";
	a = sqr(a);
	if(a != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//sqrt
	{
	mpint x = "4";
	x = sqrt(x);
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//n_root
	{
	mpint x = "4";
	x = n_root(x, 2);
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//mul_2
	{
	mpint x = "2";
	x = mul_2(x);
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//div_2
	{
	mpint x = "2";
	x = div_2(x);
	if(x != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//mul_2d
	{
	mpint x = "2";
	x = mul_2d(x, 2);
	if(x != "8"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//div_2d
	{
	mpint a = "2", c, d;
	div_2d(a, 1, c, d);
	if(c != "1"){
		LOGGER(logger::utest); ++fail;
	}
	if(d != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//mod_2d
	{
	mpint a = "2";
	a = mod_2d(a, 1);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//addmod
	{
	mpint a = "1", b = "1", c = "2";
	a = addmod(a, b, c);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//submod
	{
	mpint a = "3", b = "1", c = "2";
	a = submod(a, b, c);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//mulmod
	{
	mpint a = "2", b = "1", c = "2";
	a = mulmod(a, b, c);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//sqrmod
	{
	mpint a = "2", b = "2";
	a = sqrmod(a, b);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//invmod
	{
	mpint a = "1", b = "1";
	a = sqrmod(a, b);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//exptmod
	{
	mpint a = "4", b = "1", c = "2";
	a = exptmod(a, b, c);
	if(a != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//is_prime
	{
	mpint x = "4";
	if(is_prime(x)){
		LOGGER(logger::utest); ++fail;
	}
	}

	//next_prime
	{
	mpint x = "1";
	x = next_prime(x);
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//gcd
	{
	mpint x = "4", y = "8";
	x = gcd(x, y);
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//lcm
	{
	mpint x = "4", y = "8";
	x = lcm(x, y);
	if(x != "8"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//random_prime
	{
	mpint x = mpa::random_prime(16);
	if(!mpa::is_prime(x)){
		LOGGER(logger::utest); ++fail;
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
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator +
	{
	mpint x = "2", y = "2";
	x = x+y;
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator -
	{
	mpint x = "4", y = "2";
	x = x-y;
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator *
	{
	mpint x = "2";
	x = x*x;
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator /
	{
	mpint x = "2";
	x = x/x;
	if(x != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator &
	{
	mpint x = "1", y = "2";
	x = x&x;
	if(x != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator |
	{
	mpint x = "1", y = "2";
	x = x|y;
	if(x != "3"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator ^
	{
	mpint x = "1", y = "3";
	x = x^y;
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator %
	{
	mpint x = "2", y = "2";
	x = x%y;
	if(x != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator -
	{
	mpint x = "1";
	x = -x;
	if(x != "-1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator +=
	{
	mpint x = "2";
	x += x;
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator -=
	{
	mpint x = "2";
	x -= x;
	if(x != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator *=
	{
	mpint x = "2";
	x *= x;
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator /=
	{
	mpint x = "2";
	x /= x;
	if(x != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator &=
	{
	mpint x = "2";
	x &= x;
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator |=
	{
	mpint x = "2";
	x |= x;
	if(x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator ^=
	{
	mpint x = "2";
	x ^= x;
	if(x != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator %=
	{
	mpint x = "2";
	x %= x;
	if(x != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator <<=
	{
	mpint x = "2";
	x <<= 1;
	if(x != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator >>=
	{
	mpint x = "2";
	x >>= 1;
	if(x != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator ++ (pre)
	{
	mpint x = "1";
	if(++x != "2"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator -- (pre)
	{
	mpint x = "1";
	if(--x != "0"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator ++ (post)
	{
	mpint x = "1";
	if(x++ != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator -- (post)
	{
	mpint x = "1";
	if(x-- != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator ==
	{
	mpint x = "1", y = "2";
	if(x == y){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator !=
	{
	mpint x = "1", y = "1";
	if(x != y){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator >
	{
	mpint x = "1", y = "2";
	if(x > y){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator <
	{
	mpint x = "2", y = "1";
	if(x < y){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator >=
	{
	mpint x = "1", y = "2";
	if(x >= y){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator <=
	{
	mpint x = "2", y = "1";
	if(x <= y){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator <<
	{
	mpint x = "2";
	if(x << 1 != "4"){
		LOGGER(logger::utest); ++fail;
	}
	}

	//operator >>
	{
	mpint x = "2";
	if(x >> 1 != "1"){
		LOGGER(logger::utest); ++fail;
	}
	}

	return fail;
}
