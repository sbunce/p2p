//include
#include <convert.hpp>
#include <logger.hpp>

//standard
#include <string>

int main()
{
	//endianness
	std::string binary = convert::encode<int>(1);
	if(binary[0] != 0){
		LOGGER; exit(1);
	}

	//encode/decode
	if(convert::decode<int>(convert::encode<int>(10)) != 10){
		LOGGER; exit(1);
	}

	//hex_to_bin -> bin_to_hex
	std::string hex = "0123456789ABCDEF";
	std::string bin;
	if(!convert::hex_to_bin(hex, bin)){
		LOGGER; exit(1);
	}
	std::string hex_conv = convert::bin_to_hex(bin);
	if(hex != hex_conv){
		LOGGER; exit(1);
	}

	//size comparisons
	std::string size_0 = convert::size_SI(1024);
	std::string size_1 = convert::size_SI(512);
	if(convert::size_SI_cmp(size_0, size_1) != 1){
		LOGGER; exit(1);
	}
	size_0 = convert::size_SI(512);
	size_1 = convert::size_SI(1024);
	if(convert::size_SI_cmp(size_0, size_1) != -1){
		LOGGER; exit(1);
	}
}
