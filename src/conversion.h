#ifndef H_CONVERSION
#define H_CONVERSION

//std
#include <string>

class conversion
{
public:
	conversion();

	/*
	encode_int - converts 32bit unsigned int to 4 byte big endian int
	decode_int - converts 4 byte big endian int to 32bit unsigned int
	*/
	std::string encode_int(const unsigned int & number);
	unsigned int decode_int(std::string encoded);

private:
	enum endianness { BIG_ENDIAN_ENUM, LITTLE_ENDIAN_ENUM };

	endianness ENDIANNESS;

	union int_char{
		unsigned int num;
		unsigned char byte[sizeof(unsigned int)];
	};

	/*
	check_endianness - returns what endianness system is
	*/
	endianness check_endianness();
};
#endif
