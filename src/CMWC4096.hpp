/*
Thanks to Dr. George Marsaglia of Washington State University for the PRNG.

The period is 10^39460.877333. This PRNG is not cryptographically secure.

Proof:

Let b=2^32-1,  p=18782*b^4096+1 and k=p-1. It may take many hours to verify,
(and many more hours for the original search to find p), but b^k=1 mod p and
b^(k/2) != 1 mod p and b^(k/9391) != 1 mod p for 2 and 9391, the two prime
divisors of p-1,

It follows that p is a prime for which b is a primitive root. The period of the
base-b expansion of 1/p is p-1; more generally, the period of the base-b
expansion of k/p is the order of b mod p.  (see, for example, Marsaglia and
Zaman, `` A new class of random number generators'', (1991) Annals of Applied
Probability} V 1, No. 3, 462--480, which merely states what seems to have long
been known in number theory.

Thus the base-b expansion  of 1/p, for b=2^32-1, has period
18782*b^4096 = 10^39460.877333..., and this period is more than 10^33000 times
as long as that of the Mersenne Twister.

The C function CMWC4096 is a complimentary-multiply-with-carry RNG, which
generates a sequence of pairs x_n,c_n by means of the recursions
x_n=(b-1)-(ax_{n-r}+c_{n-1} \bmod b,
c_n=\lfloor[(ax_{n-r}+c_{n-1})/b]\rfloor.

(TeX notation).

You will have to verify that the C function CMWC4096 does this for b=2^32-1.

A description of methods for producing, in reverse order, the base-b digits of a
rational k/p is in the above paper for add-with-carry, and its extension to
multipy-with-carry is described, for example, in
http://tbf.coe.wayne.edu/jmasm/vol2_no1.pdf.

You might also want to read the postscript file mwc1.ps in

The Marsaglia Random Number CDROM including "The Diehard Battery of tests pf
Randomness".

http://www.cs.hku.hk/~diehard/
*/

#ifndef H_CMWC4096
#define H_CMWC4096

//boost
#include <boost/utility.hpp>

//custom
#include "global.hpp"

//std
#include <cassert>
#include <string>

class CMWC4096 : private boost::noncopyable
{
public:
	CMWC4096();

	/*
	extract_bytes - add random bytes (in increments of 4) until bytes is num long
	get_num       - return a random unsigned integer
	seed          - seed the PRNG, must be called before anything else
	*/
	void extract_bytes(std::string & bytes, const int num);
	boost::uint32_t get_num();
	void seed(const std::string & bytes);

private:
	enum state{
		waiting_for_seed,
		ready_to_generate
	};
	state State;

	union uint_byte{
		boost::uint32_t num;
		unsigned char byte[sizeof(boost::uint32_t)];
	};

	int idx;
	boost::uint32_t Q[4096]; //fill with random 32-bit integers
	boost::uint32_t c;       //choose random initial c < 809430660
};
#endif
