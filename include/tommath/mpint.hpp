#ifndef H_MPINT
#define H_MPINT

//include
#include <random.hpp>
#include <tommath/tommath.h>

//standard
#include <iostream>

class ltmpp_error : public std::exception
{
public:
	ltmpp_error(int c):type(c){}
	const char * what(){
		switch (type) {
		case MP_MEM: return "out of memory";
		case MP_VAL: return "invalid value";
		default    : return "unknown error";
		}
   }
private:
	int type;
};

class mpint
{
public:
	mpint();
	mpint(const char * str, int radix = 10);
	mpint(const unsigned char * bin, int len);
	mpint(const mpint & b);
	~mpint();

	/* Get Representation Functions
	to_bin:
		Returns array to binary (big-endian) of mpint. Call to_bin_size for the
		length.
	to_bin_size:
		Length of array returned by to_bin.
	to_str:
		Returns mpint in specified base (2 to 64).
	*/
   unsigned char * to_bin();
   int to_bin_size();
	char * to_str(int radix);

	void operator = (const mpint & b);
	mpint operator + (const mpint & b);
   mpint operator - (const mpint & b);
	mpint operator * (const mpint & b);
	mpint operator / (const mpint & b);
	mpint operator % (const mpint & b);
   mpint operator ^ (const mpint & b);
	mpint operator | (const mpint & b);
	mpint operator & (const mpint & b);
   mpint operator - ();
	mpint operator << (int n);
   mpint operator >> (int n);
	mpint & operator += (const mpint & b);
	mpint & operator -= (const mpint & b);
   mpint & operator *= (const mpint & b);
	mpint & operator /= (const mpint & b);
	mpint & operator %= (const mpint & b);
	mpint & operator ^= (const mpint & b);
	mpint & operator &= (const mpint & b);
	mpint & operator |= (const mpint & b);
	mpint & operator <<= (int n);
	mpint & operator >>= (int n);
   mpint & operator ++ ();
	mpint & operator -- ();
	mpint & operator ++ (int);
	mpint & operator -- (int);
	mpint & operator += (const mp_digit b);
	mpint & operator -= (const mp_digit b);
	mpint & operator *= (const mp_digit b);
	mpint & operator /= (const mp_digit b);
	mpint & operator %= (const mp_digit b);
   bool operator == (const mpint & b);
   bool operator != (const mpint & b);
   bool operator > (const mpint & b);
   bool operator < (const mpint & b);
   bool operator >= (const mpint & b);
   bool operator <= (const mpint & b);
	bool operator == (const mp_digit b);
	bool operator != (const mp_digit b);
	bool operator > (const mp_digit b);
	bool operator >= (const mp_digit b);
	bool operator < (const mp_digit b);
	bool operator <= (const mp_digit b);

	/* Math Functions
	gcd:
		Greatest common denominator.
	lcm:
		Least common multiple.
	invmod:
		Inverse modulo.
	exptmod:
		(this number)**y mod(p)
	*/
   mpint gcd(const mpint & b);
	mpint lcm(const mpint & b);
	mpint invmod(const mpint & b);
	mpint exptmod(const mpint & y, const mpint & p);

	/* Prime Generation
	is_prime:
		Returns true if mpint is prime.
	random:
		Returns a random mpint of the specified size (bytes).
	random_prime:
		Returns a random prime mpint of the specified size (bytes).
	*/
	bool is_prime();
	static mpint random(const int bytes);
	static mpint random_prime(const int bytes);

	//prints mpint in base 10
	friend std::ostream & operator << (std::ostream & lval, mpint rval)
	{
		return lval << rval.to_str(10);
	}

private:
	mp_int data;
	char * rstr;
};
#endif
