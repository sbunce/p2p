//include
#include <convert.hpp>
#include <logger.hpp>

//standard
#include <string>

int fail(0);

int main()
{
	//encode/decode
	if(convert::decode<int>(convert::encode<int>(10)) != 10){
		LOGGER; ++fail;
	}

	//encode_VLI/decode_VLI
	if(convert::VLI_size(1) != 1){
		LOGGER; ++fail;
	}
	if(convert::VLI_size(255) != 1){
		LOGGER; ++fail;
	}
	if(convert::VLI_size(256) != 2){
		LOGGER; ++fail;
	}
	if(convert::decode_VLI(convert::encode_VLI(10, 11)) != 10){
		LOGGER; ++fail;
	}

	//hex_to_bin -> bin_to_hex
	std::string hex = "0123456789ABCDEF";
	if(convert::bin_to_hex(convert::hex_to_bin(hex)) != hex){
		LOGGER; ++fail;
	}

	//size comparisons
	std::string size_0 = convert::size_SI(1024);
	std::string size_1 = convert::size_SI(512);
	if(convert::size_SI_cmp(size_0, size_1) != 1){
		LOGGER; ++fail;
	}
	size_0 = convert::size_SI(512);
	size_1 = convert::size_SI(1024);
	if(convert::size_SI_cmp(size_0, size_1) != -1){
		LOGGER; ++fail;
	}
	return fail;
}
