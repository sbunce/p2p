#ifndef H_BOOST_MPINT
#define H_BOOST_MPINT

//include
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

//standard
#include <iostream>
#include <string>

//predecl for PIMPL
#ifndef BN_H_
class mp_int;
#endif

namespace mpa{

class out_of_memory : public std::exception
{
public:
	const char * what(){ return "boost::mpa out of memory"; }
};

class invalid_value : public std::exception
{
public:
	const char * what(){ return "boost::mpa invalid value"; }
};

class mpint
{
public:
	mpint();
	mpint(const mpint & M);
	//initialize from str in specified base
	mpint(const char * str, const int radix = 10);
	//initialize from big endian encoded bytes
	mpint(const unsigned char * bin, const int len);
	~mpint();

	/*
	bin:
		Returns binary version (big-endian) of mpint. Specify bytes to pad the
		binary form with zeros to a certain size.
	str:
		Returns mpint encoded in specified radix.
	*/
   std::string bin(std::size_t bytes = 0) const;
	std::string str(const int radix = 10) const;

	//operators
	mpint & operator = (const mpint & b);
	mpint operator + (const mpint & b) const;
	mpint operator - (const mpint & b) const;
	mpint operator * (const mpint & b) const;
	mpint operator / (const mpint & b) const;
	mpint operator & (const mpint & b) const;
	mpint operator | (const mpint & b) const;
	mpint operator ^ (const mpint & b) const;
	mpint operator % (const mpint & b) const;
	mpint operator - () const;
	mpint & operator += (const mpint & b);
	mpint & operator -= (const mpint & b);
	mpint & operator *= (const mpint & b);
	mpint & operator /= (const mpint & b);
	mpint & operator &= (const mpint & b);
	mpint & operator |= (const mpint & b);
	mpint & operator ^= (const mpint & b);
	mpint & operator %= (const mpint & b);
	mpint & operator <<= (const int n);
	mpint & operator >>= (const int n);
	mpint & operator ++ ();
	mpint & operator -- ();
	mpint operator ++ (int);
	mpint operator -- (int);
	bool operator == (const mpint & b) const;
	bool operator != (const mpint & b) const;
	bool operator > (const mpint & b) const;
	bool operator < (const mpint & b) const;
	bool operator >= (const mpint & b) const;
	bool operator <= (const mpint & b) const;
	mpint operator << (const int n) const;
	mpint operator >> (const int n) const;

	//ostream
	friend std::ostream & operator << (std::ostream & lval, const mpint & b)
	{
		return lval << b.str();
	}

	//friends
	friend mpint sqr(const mpint & a);
	friend mpint sqrt(const mpint & a);
	friend mpint n_root(const mpint & a, const boost::uint32_t & b);
	friend mpint mul_2(const mpint & a);
	friend mpint div_2(const mpint & a);
	friend mpint mul_2d(const mpint & a, const boost::uint32_t b);
	friend void div_2d(const mpint & a, const boost::uint32_t b, mpint & c, mpint & d);
	friend mpint mod_2d(const mpint & a, const boost::uint32_t b);
	friend mpint addmod(const mpint & a, const mpint & b, const mpint & c);
	friend mpint submod(const mpint & a, const mpint & b, const mpint & c);
	friend mpint mulmod(const mpint & a, const mpint & b, const mpint & c);
	friend mpint sqrmod(const mpint & a, const mpint & b);
	friend mpint invmod(const mpint & a, const mpint & b);
	friend mpint exptmod(const mpint & a, const mpint & b, const mpint & c);
	friend bool is_prime(const mpint & a, const int t);
	friend mpint next_prime(const mpint & a, const int t, const bool bbs_style);
	friend mpint gcd(const mpint & a, const mpint & b);
	friend mpint lcm(const mpint & a, const mpint & b);
	friend mpint random_prime(const std::size_t bytes);

private:
	boost::shared_ptr<mp_int> data;
};

//squares a
mpint sqr(const mpint & a);

//square root a
mpint sqrt(const mpint & a);

//a^(1/b)
mpint n_root(const mpint & a, const boost::uint32_t & b);

//2a
mpint mul_2(const mpint & a);

//a/2
mpint div_2(const mpint & a);

//(2^b)a
mpint mul_2d(const mpint & a, const boost::uint32_t b);

//c = a/(2^b), d = a % (2^b)
void div_2d(const mpint & a, const boost::uint32_t b, mpint & c, mpint & d);

//a % (2^b)
mpint mod_2d(const mpint & a, const boost::uint32_t b);

//(a+b) % c
mpint addmod(const mpint & a, const mpint & b, const mpint & c);

//(a-b) % c
mpint submod(const mpint & a, const mpint & b, const mpint & c);

//(ab) % c
mpint mulmod(const mpint & a, const mpint & b, const mpint & c);

//(a^2) % b
mpint sqrmod(const mpint & a, const mpint & b);

//a^(-1) % b
mpint invmod(const mpint & a, const mpint & b);

//a^b % c
mpint exptmod(const mpint & a, const mpint & b, const mpint & c);

//returns true if prime, t = Miller-Rabin tests
bool is_prime(const mpint & a, const int t = 10);

//returns next prime, t = Miller-Rabin tests, bbs_style true = congruent to 3 % 4
mpint next_prime(const mpint & a, const int t = 10, const bool bbs_style = false);

//greatest common denominator
mpint gcd(const mpint & a, const mpint & b);

//least common multiple
mpint lcm(const mpint & a, const mpint & b);

//random prime number of specified size
mpint random_prime(const std::size_t bytes);

//random number
mpint random(const int bytes);

}//end of namespace mpint
#endif
