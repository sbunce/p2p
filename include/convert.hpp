#ifndef H_CONVERT
#define H_CONVERT

//boost
#include <boost/cstdint.hpp>

//std
#include <algorithm>
#include <cassert>
#include <cstring>
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

	static bool hex_to_bin(const std::string & hex, std::string & bin)
	{
		//verify size requirements
		if(hex.empty() || hex.size() < 2 || hex.size() % 2 == 1){
			return false;
		}

		int index = 0;
		while(index != hex.size()){
			if((hex[index] < 48 || hex[index] > 57) && (hex[index] < 65 || hex[index] > 70)){
				return false;
			}
			char ch = ((hex[index] >= 'A') ? (hex[index] - 'A' + 10) : (hex[index] - '0')) << 4;
			++index;
			if((hex[index] < 48 || hex[index] > 57) && (hex[index] < 65 || hex[index] > 70)){
				return false;
			}
			bin += ch + ((hex[index] >= 'A') ? (hex[index] - 'A' + 10) : (hex[index] - '0'));
			++index;
		}
		return true;
	}

	static std::string bin_to_hex(const std::string & bin)
	{
		static const char HEX[] = "0123456789ABCDEF";
		std::string hex;
		int size = bin.size();
		for(int x=0; x<size; ++x){
			hex += HEX[(int)((bin[x] >> 4) & 15)];
			hex += HEX[(int)(bin[x] & 15)];
		}
		return hex;
	}

//DEBUG, needs to be cleaned up
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

//DEBUG, needs to be cleaned up
	/*
	Given two strings obtained from size_unit_select, returns less than (-1),
	equal to (0), or greater than (1). Just like strcmp.

	This function doesn't do any converting but it's related to size_SI so it
	can go in the convert namespace.
	*/
	static int size_SI_cmp(const std::string & left, const std::string & right)
	{
		if(left == right){
			return 0;
		}else if(left.find("tB") != std::string::npos){
			if(right.find("tB") != std::string::npos){
				//both are tB, compare
				std::string left_temp(left, 0, left.find("tB"));
				std::string right_temp(right, 0, right.find("tB"));
				left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'), left_temp.end());
				right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'), right_temp.end());
				return std::strcmp(left_temp.c_str(), right_temp.c_str());		
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
				return std::strcmp(left_temp.c_str(), right_temp.c_str());		
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
				return std::strcmp(left_temp.c_str(), right_temp.c_str());		
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
				return std::strcmp(left_temp.c_str(), right_temp.c_str());		
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
				return std::strcmp(left_temp.c_str(), right_temp.c_str());		
			}
		}else{
			//unrecognized size
			return 0;
		}
	}
}
#endif
