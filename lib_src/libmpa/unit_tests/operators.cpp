//include
#include <logger.hpp>
#include <mpa.hpp>

int fail(0);

using namespace mpa;

int main()
{
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
