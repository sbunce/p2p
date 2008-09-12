#ifndef H_CONVERT
#define H_CONVERT

//custom
#include "global.h"

//std
#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>

namespace convert
{
	/*
	Used by functions within the convert namespace. Should be of no value outside
	the convert namespace.
	*/
	enum { BIG_ENDIAN_ENUM, LITTLE_ENDIAN_ENUM };
	union num_char {
		unsigned int num;
		unsigned char byte[sizeof(unsigned int)];
	};

	static int endianness()
	{
		//check endianness of hardware
		num_char IC = { 1 };
		if(IC.byte[0] == 1){
			return LITTLE_ENDIAN_ENUM;
		}else{
			return BIG_ENDIAN_ENUM;
		}
	}

	//encode - number -> bytes (encoded in big-endian)
	template<class T>
	static std::string encode(const T & number)
	{
		num_char IC = { number };
		std::string encoded;
		if(endianness() == LITTLE_ENDIAN_ENUM){
			for(int x=sizeof(T)-1; x>=0; --x){
				encoded += IC.byte[x];
			}
		}else{
			for(int x=0; x<sizeof(T); ++x){
				encoded += IC.byte[x];
			}
		}
		return encoded;
	}

	//decode - bytes -> number
	template<class T>
	static T decode(const std::string & encoded)
	{
		assert(encoded.size() == sizeof(T));
		num_char IC;
		if(endianness() == LITTLE_ENDIAN_ENUM){
			for(int x=0; x<sizeof(T); ++x){
				IC.byte[x] = (unsigned char)encoded[sizeof(T)-1-x];
			}
		}else{
			for(int x=0; x<sizeof(T); ++x){
				IC.byte[x] = (unsigned char)encoded[x];
			}
		}
		return IC.num;
	}

	static std::string hex_to_binary(const std::string & hex_str)
	{
		assert(hex_str.size() % 2 == 0);
		std::string binary;
		boost::uint8_t byte;
		for(int x=0; x<hex_str.size(); ++x){
			//each hex character represents half of a byte
			if(x % 2 == 0){
				//left half of byte
				byte = 0;
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
			}else{
				//right half of byte
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
				binary += (char)byte;
			}
		}
		return binary;
	}

	static std::string binary_to_hex(const std::string & binary)
	{
		char hex[] = "0123456789ABCDEF";
		std::string hash;
		for(int x=0; x<binary.size(); ++x){
			hash += hex[(int)((binary[x] >> 4) & 15)]; //left side of byte
			hash += hex[(int)(binary[x] & 15)];        //right side of byte
		}
		return hash;
	}

	/*
	Convert bytes to reasonable SI unit.
	Example 1024 -> 1kB
	*/
	static std::string size_unit_select(boost::uint64_t bytes)
	{
		std::ostringstream oss;
		if(bytes < 1024){
			oss << bytes << "B";
		}else if(bytes >= 1024*1024*1024){
			oss << std::fixed << std::setprecision(2) << bytes / (double)(1024*1024*1024) << "gB";
		}else if(bytes >= 1024*1024){
			oss << std::fixed << std::setprecision(1) << bytes / (double)(1024*1024) << "mB";
		}else if(bytes >= 1024){
			oss << std::fixed << std::setprecision(0) << bytes / (double)1024 << "kB";
		}
		return oss.str();
	}
}
#endif
