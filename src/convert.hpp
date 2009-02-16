#ifndef H_CONVERT
#define H_CONVERT

//custom
#include "global.hpp"

//std
#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>

namespace convert
{
	//returns endianness of hardware
	enum { BIG_ENDIAN_ENUM, LITTLE_ENDIAN_ENUM };
	static int endianness()
	{
		union{
			unsigned num;
			unsigned char byte[sizeof(unsigned)];
		} test;
		test.num = 1;
		if(test.byte[0] == 1){
			return LITTLE_ENDIAN_ENUM;
		}else{
			return BIG_ENDIAN_ENUM;
		}
	}

	//encode, number -> bytes (encoded in big-endian)
	template<class T>
	static std::string encode(const T & number)
	{
		union{
			T num;
			unsigned char byte[sizeof(T)];
		} NB;
		NB.num = number;
		std::string encoded;
		if(endianness() == LITTLE_ENDIAN_ENUM){
			for(int x=sizeof(T)-1; x>=0; --x){
				encoded += NB.byte[x];
			}
		}else{
			for(int x=0; x<sizeof(T); ++x){
				encoded += NB.byte[x];
			}
		}
		return encoded;
	}

	//decode, bytes -> number
	template<class T>
	static T decode(const std::string & encoded)
	{
		assert(encoded.size() == sizeof(T));
		union{
			T num;
			unsigned char byte[sizeof(T)];
		} NB;
		if(endianness() == LITTLE_ENDIAN_ENUM){
			for(int x=0; x<sizeof(T); ++x){
				NB.byte[x] = (unsigned char)encoded[sizeof(T)-1-x];
			}
		}else{
			for(int x=0; x<sizeof(T); ++x){
				NB.byte[x] = (unsigned char)encoded[x];
			}
		}
		return NB.num;
	}

	static std::string hex_to_bin(const std::string & hex_str)
	{
		assert(hex_str.size() % 2 == 0);
		std::string binary;
		boost::uint8_t byte;
		for(int x=0; x<hex_str.size(); ++x){
			if(x % 2 == 0){
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

	static std::string bin_to_hex(const std::string & binary)
	{
		static const char hex[] = "0123456789ABCDEF";
		std::string hash;
		for(int x=0; x<binary.size(); ++x){
			hash += hex[(int)((binary[x] >> 4) & 15)]; //left side of byte
			hash += hex[(int)(binary[x] & 15)];        //right side of byte
		}
		return hash;
	}

	//convert bytes to reasonable SI unit, example 1024 -> 1kB
	static std::string size_SI(boost::uint64_t bytes)
	{
		std::ostringstream oss;
		if(bytes < 1024){
			oss << bytes << "B";
		}else if(bytes >= 1024ull*1024*1024*1024){
			oss << std::fixed << std::setprecision(2) << bytes / (double)(1024ull*1024*1024*1024) << "tB";
		}else if(bytes >= 1024*1024*1024){
			oss << std::fixed << std::setprecision(2) << bytes / (double)(1024*1024*1024) << "gB";
		}else if(bytes >= 1024*1024){
			oss << std::fixed << std::setprecision(2) << bytes / (double)(1024*1024) << "mB";
		}else if(bytes >= 1024){
			oss << std::fixed << std::setprecision(1) << bytes / (double)1024 << "kB";
		}
		return oss.str();
	}

	/*
	Given two strings obtained from size_unit_select, returns less than (-1),
	equal to (0), or greater than (1). Just like strcmp.
	*/
	static int size_SI_cmp(const std::string & left, const std::string & right)
	{
		if(left == right){
			return 0;
		}else if(left.find("tB") != std::string::npos){
			if(right.find("tB") != std::string::npos){
				//both are gB, compare
				std::string left_temp(left, 0, left.find("tB"));
				std::string right_temp(right, 0, right.find("tB"));
				left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'), left_temp.end());
				right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'), right_temp.end());
				return strcmp(left_temp.c_str(), right_temp.c_str());		
			}else if(right.find("gB") != std::string::npos){
				return 1;
			}else if(right.find("mB") != std::string::npos){
				return 1;
			}else if(right.find("kB") != std::string::npos){
				return 1;
			}else{ //bytes
				return 1;
			}
		}else if(left.find("gB") != std::string::npos){
			if(right.find("tB") != std::string::npos){
				return -1;
			}else if(right.find("gB") != std::string::npos){
				//both are gB, compare
				std::string left_temp(left, 0, left.find("gB"));
				std::string right_temp(right, 0, right.find("gB"));
				left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'), left_temp.end());
				right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'), right_temp.end());
				return strcmp(left_temp.c_str(), right_temp.c_str());		
			}else if(right.find("mB") != std::string::npos){
				return 1;
			}else if(right.find("kB") != std::string::npos){
				return 1;
			}else{ //bytes
				return 1;
			}
		}else if(left.find("mB") != std::string::npos){
			if(right.find("tB") != std::string::npos){
				return -1;
			}else if(right.find("gB") != std::string::npos){
				return -1;
			}else if(right.find("mB") != std::string::npos){
				//both are mB, compare
				std::string left_temp(left, 0, left.find("mB"));
				std::string right_temp(right, 0, right.find("mB"));
				left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'), left_temp.end());
				right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'), right_temp.end());
				return strcmp(left_temp.c_str(), right_temp.c_str());		
			}else if(right.find("kB") != std::string::npos){
				return 1;
			}else{ //bytes
				return 1;
			}
		}else if(left.find("kB") != std::string::npos){
			if(right.find("tB") != std::string::npos){
				return -1;
			}else if(right.find("gB") != std::string::npos){
				return -1;
			}else if(right.find("mB") != std::string::npos){
				return -1;
			}else if(right.find("kB") != std::string::npos){
				//both are kB, compare
				std::string left_temp(left, 0, left.find("kB"));
				std::string right_temp(right, 0, right.find("kB"));
				left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'), left_temp.end());
				right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'), right_temp.end());
				return strcmp(left_temp.c_str(), right_temp.c_str());		
			}else{ //bytes
				return 1;
			}
		}else if(left.find("B") != std::string::npos){
			if(right.find("tB") != std::string::npos){
				return -1;
			}else if(right.find("gB") != std::string::npos){
				return -1;
			}else if(right.find("mB") != std::string::npos){
				return -1;
			}else if(right.find("kB") != std::string::npos){
				return -1;
			}else{ //bytes
				//both are B, compare
				std::string left_temp(left, 0, left.find("B"));
				std::string right_temp(right, 0, right.find("B"));
				left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'), left_temp.end());
				right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'), right_temp.end());
				return strcmp(left_temp.c_str(), right_temp.c_str());		
			}
		}else{
			//unrecognized size
			return 0;
		}
	}
}
#endif
