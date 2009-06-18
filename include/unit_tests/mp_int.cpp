//boost
#include <boost/mp_math/mp_int.hpp>
#include <boost/random.hpp>

//include
#include <logger.hpp>
#include <random.hpp>

//std
#include <iomanip>
#include <iostream>

/*
Function object for prime test that prime generator needs. This will generate
probable primes (probability of not being prime <= 2^-96).
*/
template <typename Engine, typename Distribution>
class tester
{
public:
	explicit tester(const Engine& e): RNG(e){}

	boost::mp_math::primality_division_test division_test;
	boost::mp_math::primality_miller_rabin_test<Distribution> Miller_Rabin;
	Engine RNG;

	bool operator()(const boost::mp_math::mp_int<> & p)
	{
		/*
		The division test is done first because it is faster and will pick up many
		non-primes. If the division test checks out we move on to Miller-Rabin.
		The number of rounds to do with Miller-Rabin is automatically set such
		that the chance of a false positive is <= 2^-96.
		*/
		return division_test(p) && Miller_Rabin(RNG, p);
	}
};

boost::mt19937 PRNG;

void seed_PRNG()
{
	const int seed_size = PRNG.state_size;
	unsigned char seed[seed_size];
	custom::random(seed, seed_size);
	//end "iterator" has to be one past end of array like normal end iterator
	unsigned char * begin = seed, * end = seed + seed_size;
	PRNG.seed(begin, end);
}

void random_prime_mp_int()
{
	typedef tester<boost::mt19937, boost::mp_math::uniform_mp_int<> > tester_type;
	typedef boost::mp_math::uniform_mp_int_bits<> distribution_type;
	boost::mp_math::safe_prime_generator<tester_type, distribution_type>
		generator(128, tester_type(PRNG));
	boost::mp_math::mp_int<> prime = generator(PRNG);
	LOGGER << "prime: " << prime;
}

void random_mp_int()
{
	boost::mp_math::uniform_mp_int_bits<> generator(128);
	boost::mp_math::mp_int<> r = generator(PRNG);
	LOGGER << "random: " << r;
}

void convert_to_bin()
{
	boost::mp_math::uniform_mp_int_bits<> generator(128);
	boost::mp_math::mp_int<> num = generator(PRNG);
	LOGGER << "num: " << num;

	LOGGER << "before";
	//std::string str = num.to_string<std::string>(std::ios::hex);
	LOGGER << "after";
}

int main()
{
	seed_PRNG();
	random_prime_mp_int();
	random_mp_int();
	convert_to_bin();
}
