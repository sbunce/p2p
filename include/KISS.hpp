/*
KISS by George Marsaglia
Period 2^125.
Not cryptographically secure.
*/
#ifndef H_KISS
#define H_KISS

//include
#include <boost/cstdint.hpp>
#include <random.hpp>

class KISS
{
public:
	KISS():
		remainder_idx(4)
	{}

	//get one byte
	unsigned char byte()
	{
		if(remainder_idx < 4){
			return remainder.byte[remainder_idx++];
		}else{
			remainder_idx = 0;
			remainder.num = extract_num();
			return remainder.byte[remainder_idx++];
		}
	}

	//get num bytes and put them in buff
	void bytes(unsigned char * buff, const int num)
	{
		int buff_idx = 0;

		//use up remainder from previous call
		for(; remainder_idx < 4 && buff_idx < num; ++remainder_idx){
			buff[buff_idx++] = remainder.byte[remainder_idx];
		}

		//extract new bytes
		while(buff_idx < num){
			remainder.num = extract_num();
			for(remainder_idx=0; remainder_idx < 4 && buff_idx < num; ++remainder_idx){
				buff[buff_idx++] = remainder.byte[remainder_idx];
			}
		}
	}

	//seed with portable_urandom
	void seed()
	{
		uint32_byte tmp;
		portable_urandom(tmp.byte, 4, NULL);
		x = tmp.num;
		portable_urandom(tmp.byte, 4, NULL);
		y = tmp.num;
		portable_urandom(tmp.byte, 4, NULL);
		z = tmp.num;
		portable_urandom(tmp.byte, 4, NULL);
		c = tmp.num;
	}

	//seed with bytes, size must be 16
	void seed(unsigned char * data, const int size)
	{
		assert(size == 16);

		uint32_byte tmp;
		for(int x=0; x<4; ++x){
			tmp.byte[x] = data[x];
		}
		x = tmp.num;

		for(int x=4; x<8; ++x){
			tmp.byte[x % 4] = data[x];
		}
		y = tmp.num;

		for(int x=8; x<12; ++x){
			tmp.byte[x % 4] = data[x];
		}
		z = tmp.num;

		for(int x=12; x<16; ++x){
			tmp.byte[x % 4] = data[x];
		}
		c = tmp.num;
	}

private:
	//to seed KISS x, y, z, and c must be set to random numbers
	boost::uint32_t x, y, z, c;
	boost::uint64_t t, a;

	union uint32_byte{
		boost::uint32_t num;
		unsigned char byte[sizeof(boost::uint32_t)];
	};

	//storage for remainder when getting bytes % 4 != 0
	int remainder_idx;
	uint32_byte remainder;

	//main logic of PRNG
	boost::uint32_t extract_num()
	{
		a = 698769069;
		x = 69069 * x + 12345;
		y ^= (y << 13);
		y ^= (y >> 17);
		y ^= (y << 5);
		t = a * z + c;
		c = (t >> 32);
		return x + y + (z = t);
	}
};
#endif
