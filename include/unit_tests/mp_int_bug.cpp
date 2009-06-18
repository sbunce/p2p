/*
Compiler: GCC 4.3.2 (Debian Lenny)
boost::mt19937 from boost 1.39
*/

#include <boost/mp_math/mp_int.hpp>
#include <iomanip>
#include <iostream>
int main()
{
	//this setup causes the crash
	boost::mt19937 PRNG;
	boost::mp_math::uniform_mp_int_bits<> generator(128);
	boost::mp_math::mp_int<> num = generator(PRNG);

	//this setup is fine
	//boost::mp_math::mp_int<> num("123");

	/*
	Giving to_string a parameter of:
	std::ios::dec - fine
	std::ios::oct - fine
	std::ios::hex - crash
	*/
//DEBUG, uncomment to enable bug
	//std::string hex = num.to_string<std::string>(std::ios::hex);
}

/* Speculation:
The generator is somehow returning a malformed mp_int. When trying to do
to_string(std::ios::hex) this malformed'ness is manifesting itself in the form
of a crash.
*/
