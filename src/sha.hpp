#ifndef H_SHA
#define H_SHA

//custom
#include "global.hpp"

//std
#include <algorithm>
#include <iostream>
#include <string>

class sha
{
public:
	/*
	It is best to reserve a buffer that is as large as the chunks you are loading
	with the load() function.
	*/
	sha();

	/*
	If you know exactly how much data you will be loading this can speed up
	things by reserving space in the load buffer.
	*/
	void reserve(const unsigned res);

	/*
	Functions to load data to be hashed.
	init - must be called before load()'ing (clears buffers and does setup)
	load - load data in chunks
	end  - must be called before retrieving the hash with hex_hash or raw_hash
	*/
	void init();                           //call before load()'ing data
	void load(const char * data, int len); //load data to hash, len in bytes
	void end();                            //finished loading data

	/*
	Functions to retrieve hash for data.
	hex_hash - returns hash in hex
	raw_hash - returns hash in binary
	           WARNING - VIOLATES ENCAPSULATION (for performance)
	*/
	std::string hex_hash();
	char * raw_hash();

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
