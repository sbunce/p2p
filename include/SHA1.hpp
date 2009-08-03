//NOT-THREADSAFE
#ifndef H_SHA1
#define H_SHA1

//include
#include <boost/cstdint.hpp>
#include <boost/detail/endian.hpp>

//standard
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

class SHA1
{
public:
	SHA1(){}

	/* Data Loading Functions
	init:
		Must be called before loading data.
	load:
		Load data in chunks.
	end:
		Must be called before retrieving the hash with hex_hash or raw_hash.
	*/
	void init()
	{
		h[0].num = 0x67452301;
		h[1].num = 0xEFCDAB89;
		h[2].num = 0x98BADCFE;
		h[3].num = 0x10325476;
		h[4].num = 0xC3D2E1F0;
		loaded_bytes = 0;
		load_buffer.clear();
	}

	void load(const char * data, int len)
	{
		//update buffer
		loaded_bytes += len;
		load_buffer.append(data, len);
		if(load_buffer.size() >= 64){
			//process available chunks and free memory
			int ready_chunks = (load_buffer.size() - (load_buffer.size() % 64)) / 64;
			for(int x=0; x<ready_chunks; ++x){
				process(x*64);
			}
			//erase processed chunks
			load_buffer.erase(0,ready_chunks*64);
		}
	}

	void end()
	{
		//append trailing 1 bit and size
		append_tail();

		//process remaining chunks
		int ready_chunks = load_buffer.size() / 64;
		for(int x=0; x<ready_chunks; ++x){
			process(x*64);
		}
		//erase processed chunks
		load_buffer.erase(0,ready_chunks*64);

		//create the final hash by concatenating the h's (also change endianness)
		for(int x=0; x<5; ++x){
			raw[x*4+3] = h[x].byte[0];
			raw[x*4+2] = h[x].byte[1];
			raw[x*4+1] = h[x].byte[2];
			raw[x*4+0] = h[x].byte[3];
		}
	}

	/*
	Precondition: Must have hashed some data and called end().
	hex_hash:
		Returns hash in hex.
	raw_hash:
		Returns hash in binary.
	*/
	std::string hex_hash()
	{
		static const char hex[] = "0123456789ABCDEF";
		std::string hash;
		for(int x=0; x<20; ++x){
			hash += hex[static_cast<int>((raw[x] >> 4) & 15)];
			hash += hex[static_cast<int>(raw[x] & 15)];
		}
		return hash;
	}

	const char * raw_hash()
	{
		return raw;
	}

private:
	//holds last generated raw hash
	char raw[20];

	//used by load()
	boost::uint64_t loaded_bytes; //total bytes input
	std::string load_buffer;      //holds data input

	//makes conversion from int to bytes easier
	union sha_uint32_t{
		boost::uint32_t num;
		char byte[sizeof(boost::uint32_t)];
	};
	union sha_uint64_t{
		boost::uint64_t num;
		unsigned char byte[sizeof(boost::uint64_t)];
	};

	sha_uint32_t w[80]; //holds expansion of a chunk
	sha_uint32_t h[5];  //collected hash values

	inline void append_tail()
	{
		load_buffer += static_cast<char>(128); //append bit 1

		//append k zero bits such that size (in bits) is congruent to 448 mod(512)
		if(load_buffer.size() % 64 < 56){
			load_buffer.append(56 - (load_buffer.size() % 64), static_cast<char>(0));
		}else if(load_buffer.size() % 64 > 56){
			load_buffer.append(64 + (load_buffer.size() % 64), static_cast<char>(0));
		}

		//append size of original message (in bits) encoded as a 64bit big-endian
		sha_uint64_t IC = { loaded_bytes * 8 };
		#ifdef BOOST_LITTLE_ENDIAN
		for(int x=7; x>=0; --x){
			load_buffer += IC.byte[x];
		}
		#else
		for(int x=0; x<8; ++x){
			load_buffer += IC.byte[x];
		}
		#endif
	}
	void process(const int & chunk_start)
	{
		//break 512bit chunk in to 16, 32 bit pieces
		for(int x=0; x<16; ++x){
			load_buffer.copy(w[x].byte, 4, chunk_start + x*4);
			#ifdef BOOST_LITTLE_ENDIAN
			std::reverse(w[x].byte, w[x].byte+4);
			#endif
		}

		//extend the 16 pieces to 80
		for(int x=16; x<80; ++x){
			w[x].num = rotate_left((w[x-3].num ^ w[x-8].num ^ w[x-14].num ^ w[x-16].num), 1);
		}

		boost::uint32_t a, b, c, d, e;
		a = h[0].num;
		b = h[1].num;
		c = h[2].num;
		d = h[3].num;
		e = h[4].num;

		for(int x=0; x<80; ++x){
			boost::uint32_t f, k;
			if(0 <= x && x <= 19){
				f = (b & c) | ((~b) & d);
				k = 0x5A827999;
			}else if(20 <= x && x <= 39){
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			}else if(40 <= x && x <= 59){
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			}else if(60 <= x && x <= 79){
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}

			boost::uint32_t temp = rotate_left(a, 5) + f + e + k + w[x].num;
			e = d;
			d = c;
			c = rotate_left(b, 30);
			b = a;
			a = temp;
		}

		h[0].num += a;
		h[1].num += b;
		h[2].num += c;
		h[3].num += d;
		h[4].num += e;
	}

	//bit rotation
	boost::uint32_t rotate_right(boost::uint32_t data, int bits)
	{
		return ((data >> bits) | (data << (32 - bits)));
	}
	boost::uint32_t rotate_left(boost::uint32_t data, int bits)
	{
		return ((data << bits) | (data >> (32 - bits)));
	}
};
#endif
