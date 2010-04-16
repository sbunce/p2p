//include
#include <convert.hpp>
#include <RC4.hpp>

//standard
#include <iostream>
#include <string>

int fail(0);

int main()
{
	RC4 PRNG;
	PRNG.seed((unsigned char *)"Key", 3);
	std::string data = "Plaintext";
	for(int x=0; x<data.size(); ++x){
		data[x] = data[x] ^ PRNG.byte();
	}
	if(convert::bin_to_hex(data) != "857047028B192029FD"){
		LOG; ++fail;
	}
	return fail;
}
