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
	std::string encoded_number;
	std::bitset<32> bs = number;
	std::bitset<32> bs_temp;

	for(int x=0; x<4; ++x){
		bs_temp = bs;
		bs_temp <<= 8*x;
		bs_temp >>= 24;
		encoded_number += (unsigned char)bs_temp.to_ulong();
	}

	return encoded_number;
}

inline unsigned int decode_int(std::string recv_buff)
{
	std::bitset<32> bs(0);
	std::bitset<32> bs_temp;

	int y = 3;
	for(int x=0; x<recv_buff.size(); ++x){
		bs_temp = (unsigned char)recv_buff[x];
		bs_temp <<= 8*y--;
		bs |= bs_temp;
	}

	return (unsigned int)bs.to_ulong();
}

}
#endif
