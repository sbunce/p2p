//include
#include <convert.hpp>
#include <logger.hpp>

//standard
#include <string>

int main()
{
	//encode/decode
	if(convert::decode<int>(convert::encode<int>(10)) != 10){
		LOGGER; exit(1);
	}

	//encode_VLI/decode_VLI
	if(convert::VLI_size(1) != 1){
		LOGGER; exit(1);
	}
	if(convert::VLI_size(255) != 1){
		LOGGER; exit(1);
	}
	if(convert::VLI_size(256) != 2){
		LOGGER; exit(1);
	}
	if(convert::decode_VLI(convert::encode_VLI(10, 11)) != 10){
		LOGGER; exit(1);
	}

	//hex_to_bin -> bin_to_hex
	std::string hex = "0123456789ABCDEF";
	if(convert::bin_to_hex(convert::hex_to_bin(hex)) != hex){
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
