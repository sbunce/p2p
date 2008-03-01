#include "conversion.h"

conversion::conversion()
{
	ENDIANNESS = check_endianness();
}

//converts 32bit unsigned int to 4 byte big endian int
std::string conversion::encode_int(const unsigned int & number)
{
	std::bitset<32> bs(number);
	int_char IC = { bs.to_ulong() };
	std::string encoded("0000");

	if(ENDIANNESS == LITTLE_ENDIAN_ENUM){
		encoded[0] = IC.byte[3];
		encoded[1] = IC.byte[2];
		encoded[2] = IC.byte[1];
		encoded[3] = IC.byte[0];
	}
	else{
		encoded[0] = IC.byte[0];
		encoded[1] = IC.byte[1];
		encoded[2] = IC.byte[2];
		encoded[3] = IC.byte[3];
	}

	return encoded;
}

//converts 4 byte big endian int to 32bit unsigned int
unsigned int conversion::decode_int(std::string encoded)
{
	#ifdef DEBUG
	if(encoded.size() != 4){
		std::cout << "conversion::decode_int(): fatal error impproper byte length\n";
		exit(1);
	}
	#endif

	int_char IC;

	if(ENDIANNESS == LITTLE_ENDIAN_ENUM){
		IC.byte[0] = (unsigned char)encoded[3];
		IC.byte[1] = (unsigned char)encoded[2];
		IC.byte[2] = (unsigned char)encoded[1];
		IC.byte[3] = (unsigned char)encoded[0];
	}
	else{
		IC.byte[0] = (unsigned char)encoded[0];
		IC.byte[1] = (unsigned char)encoded[1];
		IC.byte[2] = (unsigned char)encoded[2];
		IC.byte[3] = (unsigned char)encoded[3];
	}

	return IC.num;
}

conversion::endianness conversion::check_endianness()
{
	int_char IC = { 1U };

	if(IC.byte[0] == 1U){
		return LITTLE_ENDIAN_ENUM;
	}
	else{
		return BIG_ENDIAN_ENUM;
	}
}
