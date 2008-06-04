#ifndef H_HEX
#define H_HEX

//std
#include <cassert>
#include <string>

namespace hex
{
	static std::string hex_to_binary(const std::string & hex_str)
	{
		assert(hex_str.size() % 2 == 0);
		std::string binary;
		uint8_t byte;
		for(int x=0; x<hex_str.size(); ++x){
			//each hex character represents half of a byte
			if(x % 2 == 0){
				//first half of byte
				byte = 0;
				if(hex_str[x] == '0'){byte += 0;}
				else if(hex_str[x] == '1'){byte += 1;}
				else if(hex_str[x] == '2'){byte += 2;}
				else if(hex_str[x] == '3'){byte += 3;}
				else if(hex_str[x] == '4'){byte += 4;}
				else if(hex_str[x] == '5'){byte += 5;}
				else if(hex_str[x] == '6'){byte += 6;}
				else if(hex_str[x] == '7'){byte += 7;}
				else if(hex_str[x] == '8'){byte += 8;}
				else if(hex_str[x] == '9'){byte += 9;}
				else if(hex_str[x] == 'A'){byte += 10;}
				else if(hex_str[x] == 'B'){byte += 11;}
				else if(hex_str[x] == 'C'){byte += 12;}
				else if(hex_str[x] == 'D'){byte += 13;}
				else if(hex_str[x] == 'E'){byte += 14;}
				else if(hex_str[x] == 'F'){byte += 15;}
			}else{
				//second half of byte
				if(hex_str[x] == '0'){byte += 0;}
				else if(hex_str[x] == '1'){byte += 16;}
				else if(hex_str[x] == '2'){byte += 32;}
				else if(hex_str[x] == '3'){byte += 48;}
				else if(hex_str[x] == '4'){byte += 64;}
				else if(hex_str[x] == '5'){byte += 80;}
				else if(hex_str[x] == '6'){byte += 96;}
				else if(hex_str[x] == '7'){byte += 112;}
				else if(hex_str[x] == '8'){byte += 128;}
				else if(hex_str[x] == '9'){byte += 144;}
				else if(hex_str[x] == 'A'){byte += 160;}
				else if(hex_str[x] == 'B'){byte += 176;}
				else if(hex_str[x] == 'C'){byte += 192;}
				else if(hex_str[x] == 'D'){byte += 208;}
				else if(hex_str[x] == 'E'){byte += 224;}
				else if(hex_str[x] == 'F'){byte += 240;}
				binary += (char)byte;
			}
		}
		return binary;
	}

	static std::string binary_to_hex(const std::string & binary)
	{
		static char HEX[] = "0123456789ABCDEF";
		std::string hash;
		for(int x=0; x<binary.size(); ++x){
			hash += HEX[(int)(binary[x] & 15)];
			hash += HEX[(int)((binary[x] >> 4) & 15)];
		}
		return hash;
	}
};
#endif
