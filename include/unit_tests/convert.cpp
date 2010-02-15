//include
#include <convert.hpp>
#include <logger.hpp>

//standard
#include <string>

int fail(0);

int main()
{
	//encode/decode
	if(convert::bin_to_int<int>(convert::int_to_bin<int>(10)) != 10){
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
	if(convert::bin_VLI_to_int(convert::int_to_bin_VLI(10, 11)) != 10){
		LOGGER; ++fail;
	}

	//hex_to_bin -> bin_to_hex
	std::string hex = "0123456789ABCDEF";
	if(convert::bin_to_hex(convert::hex_to_bin(hex)) != hex){
		LOGGER; ++fail;
	}

	//size comparisons
	std::string size_0 = convert::bytes_to_SI(1024);
	std::string size_1 = convert::bytes_to_SI(512);
	if(convert::SI_cmp(size_0, size_1) != 1){
		LOGGER; ++fail;
	}
	size_0 = convert::bytes_to_SI(512);
	size_1 = convert::bytes_to_SI(1024);
	if(convert::SI_cmp(size_0, size_1) != -1){
		LOGGER; ++fail;
	}
	return fail;
}
