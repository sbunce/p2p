//include
#include <convert.hpp>
#include <logger.hpp>

//custom
#include "../settings.hpp"

//std
#include <string>

bool test_encode_decode()
{
	return convert::decode<int>(convert::encode<int>(10)) == 10;
}

bool test_hex_conversion()
{
	std::string hex = "0123456789ABCDEF";
	std::string bin;
	if(!convert::hex_to_bin(hex, bin)){
		LOGGER; exit(1);
	}
	std::string hex_conv = convert::bin_to_hex(bin);
	return hex == hex_conv;
}

//encode should put bytes in big endian
bool test_endianness()
{
	std::string binary = convert::encode<int>(1);
	return binary[0] == 0;
}

int main()
{
	if(!test_encode_decode()){
		LOGGER; exit(1);
	}
	if(!test_hex_conversion()){
		LOGGER; exit(1);
	}
	if(!test_endianness()){
		LOGGER; exit(1);
	}

	return 0;
}
