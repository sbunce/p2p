//include
#include <convert.hpp>
#include <RC4.hpp>

//std
#include <iostream>
#include <string>

int main()
{
	RC4 PRNG;
	PRNG.seed((unsigned char *)"Key", 3);
	std::string data = "Plaintext";
	for(int x=0; x<data.size(); ++x){
		data[x] = data[x] ^ PRNG.get_byte();
	}
	assert(convert::bin_to_hex(data) == "857047028B192029FD");
}
