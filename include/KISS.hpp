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
		#ifdef BOOST_LITTLE_ENDIAN
		if(remainder_idx < 4){
			return remainder.b[remainder_idx++];
		}else{
			remainder_idx = 0;
			remainder.n = extract();
			return remainder.b[remainder_idx++];
		}
		#else
		if(remainder_idx > -1){
			return remainder.b[remainder_idx--];
		}else{
			remainder_idx = 3;
			remainder.n = extract();
			return remainder.b[remainder_idx--];
		}
		#endif
	}

	//get num bytes and put them in buff
	void bytes(unsigned char * buff, const int num)
	{
		for(int x=0; x<num; ++x){
			buff[x] = byte();
		}
	}

	//returns 32bit int
	boost::uint32_t i32()
	{
		uint32_byte temp;
		for(int x=0; x<4; ++x){
			temp.b[x] = byte();
		}
		return temp.n;
	}

	//returns 64bit int
	boost::uint64_t i64()
	{
		uint64_byte temp;
		for(int x=0; x<8; ++x){
			temp.b[x] = byte();
		}
		return temp.n;
	}

	//seed with portable_urandom
	void seed()
	{
		portable_urandom(x.b, 4, NULL);
		portable_urandom(y.b, 4, NULL);
		portable_urandom(z.b, 4, NULL);
		portable_urandom(c.b, 4, NULL);
	}

	//seed with bytes, size must be 16
	void seed(unsigned char * data, const int size)
	{
		assert(size == 16);
		for(int i=0; i<16; ++i){
			if(i<4){
				x.b[i] = data[i];
			}else if(i<8){
				y.b[i % 4] = data[i];
			}else if(i<12){
				z.b[i % 4] = data[i];
			}else if(i<16){
				c.b[i % 4] = data[i];
			}
		}
	}

private:

	union uint32_byte{
		boost::uint32_t n;
		unsigned char b[sizeof(boost::uint32_t)];
	};

	union uint64_byte{
		boost::uint64_t n;
		unsigned char b[sizeof(boost::uint64_t)];
	};

	//to seed KISS x, y, z, and c must be set to random numbers
	uint32_byte x, y, z, c;
	uint64_byte t, a;

	//storage for remainder when getting bytes % 4 != 0
	int remainder_idx;
	uint32_byte remainder;

	//main logic of PRNG
	boost::uint32_t extract()
	{
		a.n = 698769069;
		x.n = 69069 * x.n + 12345;
		y.n ^= (y.n << 13);
		y.n ^= (y.n >> 17);
		y.n ^= (y.n << 5);
		t.n = a.n * z.n + c.n;
		c.n = (t.n >> 32);
		return x.n + y.n + (z.n = t.n);
	}
};
#endif
