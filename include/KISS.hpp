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
			return remainder.b[remainder_idx++];
		}else{
			remainder_idx = 0;
			remainder.n = extract();
			#ifdef BOOST_LITTLE_ENDIAN
			std::reverse(remainder.b, remainder.b + sizeof(remainder));
			#endif
			return remainder.b[remainder_idx++];
		}
	}

	//get num bytes and put them in buff
	void bytes(unsigned char * buff, const int num)
	{
		for(int x=0; x<num; ++x){
			buff[x] = byte();
		}
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
		#ifdef BOOST_LITTLE_ENDIAN
		std::reverse_copy(data + 0, data + 4, x.b);
		std::reverse_copy(data + 4, data + 8, y.b);
		std::reverse_copy(data + 8, data + 12, z.b);
		std::reverse_copy(data + 12, data + 16, c.b);
		#else
		std::copy(data + 0, data + 4, x.b);
		std::copy(data + 4, data + 8, y.b);
		std::copy(data + 8, data + 12, z.b);
		std::copy(data + 12, data + 16, c.b);
		#endif
	}

private:

	union uint32_byte{
		boost::uint32_t n;
		unsigned char b[sizeof(boost::uint32_t)];
	};

	//to seed KISS x, y, z, and c must be set to random numbers
	uint32_byte x, y, z, c;

	//storage for remainder when getting bytes % 4 != 0
	int remainder_idx;
	uint32_byte remainder;

	//main logic of PRNG
	boost::uint32_t extract()
	{
		boost::uint64_t t, a = 698769069;
		x.n = 69069 * x.n + 12345;
		y.n ^= (y.n << 13);
		y.n ^= (y.n >> 17);
		y.n ^= (y.n << 5);
		t = a * z.n + c.n;
		c.n = (t >> 32);
		return x.n + y.n + (z.n = t);
	}
};
#endif
