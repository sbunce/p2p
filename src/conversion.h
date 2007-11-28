/*various functions for converting to and from characters
encode_int - converts 32bit integer to 4 chars
decode_int - turns four char's in to a 32bit integer starting at 'begin'
*/

#ifndef H_CONVERSION
#define H_CONVERSION

#include <bitset>
#include <string>

namespace conversion
{

inline std::string encode_int(const unsigned int & number)
{
	std::string encodedNumber;
	std::bitset<32> bs = number;
	std::bitset<32> bs_temp;

	for(int x=0; x<4; ++x){
		bs_temp = bs;
		bs_temp <<= 8*x;
		bs_temp >>= 24;
		encodedNumber += (unsigned char)bs_temp.to_ulong();
	}

	return encodedNumber;
}

inline unsigned int decode_int(const int & begin, char recvBuffer[])
{
	std::bitset<32> bs(0);
	std::bitset<32> bs_temp;

	int y = 3;
	for(int x=begin; x<begin+4; ++x){
		bs_temp = (unsigned char)recvBuffer[x];
		bs_temp <<= 8*y--;
		bs |= bs_temp;
	}

	return (unsigned int)bs.to_ulong();
}

}
#endif
