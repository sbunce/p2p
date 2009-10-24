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
		h[0].n = 0x67452301;
		h[1].n = 0xEFCDAB89;
		h[2].n = 0x98BADCFE;
		h[3].n = 0x10325476;
		h[4].n = 0xC3D2E1F0;
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

		//create the final hash by concatenating the h's
		for(int x=0; x<5; ++x){
			raw[x*4+3] = h[x].b[0];
			raw[x*4+2] = h[x].b[1];
			raw[x*4+1] = h[x].b[2];
			raw[x*4+0] = h[x].b[3];
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
		static const char * const hex = "0123456789ABCDEF";
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
	//holds lastest generated raw hash
	char raw[20];

	//used by load()
	boost::uint64_t loaded_bytes; //total bytes input
	std::string load_buffer;      //holds data input

	//makes conversion from int to bytes easier
	union uint32_byte{
		boost::uint32_t n;
		char b[sizeof(boost::uint32_t)];
	};
	union uint64_byte{
		boost::uint64_t n;
		unsigned char b[sizeof(boost::uint64_t)];
	};

	uint32_byte w[80]; //holds expansion of a chunk
	uint32_byte h[5];  //collected hash values

	inline void append_tail()
	{
		load_buffer += static_cast<char>(128); //append bit 1

		//append k zero bits such that size (in bits) is congruent to 448 mod(512)
		if(load_buffer.size() % 64 < 56){
			load_buffer.append(56 - (load_buffer.size() % 64), static_cast<char>(0));
		}else if(load_buffer.size() % 64 > 56){
			load_buffer.append(64 + (load_buffer.size() % 64), static_cast<char>(0));
		}

		//append size of original message (in bits) encoded as 64bit big-endian
		uint64_byte IC;
		IC.n = loaded_bytes * 8;
		load_buffer.resize(load_buffer.size() + 8);
		#ifdef BOOST_LITTLE_ENDIAN
		std::reverse_copy(IC.b, IC.b + sizeof(IC), load_buffer.end() - 8);
		#elif
		std::copy(IC.b, IC.b + sizeof(IC), load_buffer.end() - 8);
		#endif
	}

	void process(const int & chunk_start)
	{
		//break 512bit chunk in to 16, 32 bit pieces
		for(int x=0; x<16; ++x){
			int offset = chunk_start + x * 4;
			#ifdef BOOST_LITTLE_ENDIAN
			std::reverse_copy(load_buffer.begin() + offset,
				load_buffer.begin() + offset + 4, w[x].b);
			#elif
			std::copy(load_buffer.begin() + offset,
				load_buffer.begin() + offset + 4, w[x].b);
			#endif
		}

		//extend the 16 pieces to 80
		for(int x=16; x<80; ++x){
			w[x].n = rotate_left((w[x-3].n ^ w[x-8].n ^ w[x-14].n ^ w[x-16].n), 1);
		}

		boost::uint32_t a, b, c, d, e;
		a = h[0].n;
		b = h[1].n;
		c = h[2].n;
		d = h[3].n;
		e = h[4].n;

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

			boost::uint32_t temp = rotate_left(a, 5) + f + e + k + w[x].n;
			e = d;
			d = c;
			c = rotate_left(b, 30);
			b = a;
			a = temp;
		}

		h[0].n += a;
		h[1].n += b;
		h[2].n += c;
		h[3].n += d;
		h[4].n += e;
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
