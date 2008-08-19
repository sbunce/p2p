#ifndef H_SHA
#define H_SHA

//custom
#include "global.h"

//std
#include <algorithm>
#include <iostream>
#include <string>

class sha
{
public:
	sha(unsigned int reserve_buffer = 64);

	//functions to load data to be hashed
	void init();                           //call before load()'ing data
	void load(const char * data, int len); //load data to hash, len in bytes
	void end();                            //finished loading data

	//access hashes after end()
	std::string hex_hash(); //base 16 (slow, better to use raw if possible)
	char * raw_hash();      //raw bytes (20 bytes)
	char * raw_hash_no_null(); //if hash bytes are all 0 the last byte is changed to 1

	static const int HASH_LENGTH = 20;

private:
	//holds last generated raw hash
	char raw[20];

	//used by load()
	boost::uint64_t loaded_bytes; //total bytes input
	std::string load_buffer;      //holds data input

	enum endianness { BIG_ENDIAN_ENUM, LITTLE_ENDIAN_ENUM };
	endianness ENDIANNESS; //check done in ctor to set this

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

	inline void append_tail();
	inline void process(const int & chunk_start);

	//bit rotation
	inline boost::uint32_t rotate_right(boost::uint32_t data, int bits)
	{
		return ((data >> bits) | (data << (32 - bits)));
	}
	inline boost::uint32_t rotate_left(boost::uint32_t data, int bits)
	{
		return ((data << bits) | (data >> (32 - bits)));
	}
};
#endif
