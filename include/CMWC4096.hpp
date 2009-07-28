/*
CMWC4096 by George Marsaglia
Period 10^39460.877333.
Not cryptographically secure.

This PRNG seeds itself in the ctor.
*/

#ifndef H_CMWC4096
#define H_CMWC4096

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <random.hpp>

class CMWC4096 : private boost::noncopyable
{
public:
	CMWC4096():
		idx(4095),
		remainder_idx(4)
	{
		portable_urandom(reinterpret_cast<unsigned char *>(Q), 4096 * 4, NULL);

		//set c to random number between 0 and 809430660
		uint_byte UB;
		portable_urandom(UB.byte, sizeof(UB.byte), NULL);
		c = UB.num & 809430660;
	}

	//fill buff with num random bytes
	void bytes(unsigned char * buff, const int num)
	{
		int buff_idx = 0;
		for(; remainder_idx<4 && buff_idx < num; ++remainder_idx){
			buff[buff_idx++] = remainder.byte[remainder_idx];
		}
		while(buff_idx < num){
			remainder.num = extract_num();
			for(remainder_idx=0; remainder_idx<4 && buff_idx < num; ++remainder_idx){
				buff[buff_idx++] = remainder.byte[remainder_idx];
			}
		}
	}

private:
	int idx;
	boost::uint32_t Q[4096]; //fill with random 32-bit integers
	boost::uint32_t c;       //choose random initial c < 809430660

	union uint_byte{
		boost::uint32_t num;
		unsigned char byte[sizeof(boost::uint32_t)];
	};

	//storage for remainder when getting bytes % 4 != 0
	int remainder_idx;
	uint_byte remainder;

	//return a random unsigned integer
	boost::uint32_t extract_num()
	{
		boost::uint64_t t, a = 18782LL;
		boost::uint32_t x, r = 0xfffffffe;
		idx = (idx + 1) & 4095;
		t = a * Q[idx] + c;
		c = (t >> 32);
		x = t + c;
		if(x < c){ ++x; ++c; }
		return (Q[idx] = r - x);
	}
};
#endif
