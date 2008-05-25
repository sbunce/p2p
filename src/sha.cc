//std
#include <algorithm>

#include "sha.h"

const int sha::HASH_LENGTH = 20;
static char hex[] = "0123456789ABCDEF";

sha::sha(unsigned int reserve_buffer)
{
	load_buffer.reserve(reserve_buffer);

	//check endianness of hardware
	sha_uint32_t IC = { 1 };
	if(IC.byte[0] == 1){
		ENDIANNESS = LITTLE_ENDIAN_ENUM;
	}else{
		ENDIANNESS = BIG_ENDIAN_ENUM;
	}
}

void sha::init()
{
	h[0].num = 0x67452301;
	h[1].num = 0xEFCDAB89;
	h[2].num = 0x98BADCFE;
	h[3].num = 0x10325476;
	h[4].num = 0xC3D2E1F0;

	loaded_bytes = 0;
}

void sha::load(const char * data, int len)
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

void sha::end()
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

inline void sha::append_tail()
{
	load_buffer += (char)128; //append bit 1

	//append k zero bits such that size (in bits) is congruent to 448 mod(512)
	if(load_buffer.size() % 64 < 56){
		load_buffer.append(56 - (load_buffer.size() % 64), (char)0);
	}else if(load_buffer.size() % 64 > 56){
		load_buffer.append(64 + (load_buffer.size() % 64), (char)0);
	}

	//append size of original message (in bits) encoded as a 64bit big-endian int
	sha_uint64_t IC = { loaded_bytes*8 };
	if(ENDIANNESS == LITTLE_ENDIAN_ENUM){
		for(int x=7; x>=0; --x){
			load_buffer += IC.byte[x];
		}
	}else{
		for(int x=0; x<8; ++x){
			load_buffer += IC.byte[x];
		}
	}
}

inline void sha::process(const int & chunk_start)
{
	//break 512bit chunk in to 16, 32 bit pieces
	for(int x=0; x<16; ++x){
		load_buffer.copy(w[x].byte, 4, chunk_start + x*4);
		if(ENDIANNESS == LITTLE_ENDIAN_ENUM){
			std::reverse(w[x].byte, w[x].byte+4);
		}
	}

	//extend the 16 pieces to 80
	for(int x=16; x<80; ++x){
		w[x].num = rotate_left((w[x-3].num ^ w[x-8].num ^ w[x-14].num ^ w[x-16].num), 1);
	}

	uint32_t a, b, c, d, e;
	a = h[0].num;
	b = h[1].num;
	c = h[2].num;
	d = h[3].num;
	e = h[4].num;

	for(int x=0; x<80; ++x){
		uint32_t f, k;
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

		uint32_t temp = rotate_left(a, 5) + f + e + k + w[x].num;
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

std::string sha::hex_hash()
{
	std::string hash;
	for(int x=0; x<20; ++x){
		hash += hex[(int)((raw[x] >> 4) & 15)];
		hash += hex[(int)(raw[x] & 15)];
	}
	return hash;
}

char * sha::raw_hash()
{
	return raw;
}
