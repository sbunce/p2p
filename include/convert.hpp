#ifndef H_CONVERT
#define H_CONVERT

//include
#include <boost/cstdint.hpp>
#include <logger.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

class convert
{
public:
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
				NB.byte[x] = static_cast<unsigned char>(encoded[sizeof(T)-1-x]);
			}
		}else{
			for(int x=0; x<sizeof(T); ++x){
				NB.byte[x] = static_cast<unsigned char>(encoded[x]);
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
			if((hex[index] < 48 || hex[index] > 57) && (hex[index] < 65
				|| hex[index] > 70))
			{
				return false;
			}
			char ch = ((hex[index] >= 'A') ?
				(hex[index] - 'A' + 10) : (hex[index] - '0')) << 4;
			++index;
			if((hex[index] < 48 || hex[index] > 57) && (hex[index] < 65
				|| hex[index] > 70))
			{
				return false;
			}
			bin += ch + ((hex[index] >= 'A') ?
				(hex[index] - 'A' + 10) : (hex[index] - '0'));
			++index;
		}
		return true;
	}

	static std::string bin_to_hex(const std::string & bin)
	{
		const char HEX[] = "0123456789ABCDEF";
		std::string hex;
		int size = bin.size();
		for(int x=0; x<size; ++x){
			hex += HEX[static_cast<int>((bin[x] >> 4) & 15)];
			hex += HEX[static_cast<int>(bin[x] & 15)];
		}
		return hex;
	}

	//convert bytes to reasonable SI unit, example 1024 -> 1kB
	static std::string size_SI(boost::uint64_t bytes)
	{
		std::ostringstream oss;
		if(bytes < 1024){
			oss << bytes << "B";
		}else if(bytes >= 1024ull*1024*1024*1024){
			oss << std::fixed << std::setprecision(2)
				<< bytes / static_cast<double>(1024ull*1024*1024*1024) << "tB";
		}else if(bytes >= 1024*1024*1024){
			oss << std::fixed << std::setprecision(2)
				<< bytes / static_cast<double>(1024*1024*1024) << "gB";
		}else if(bytes >= 1024*1024){
			oss << std::fixed << std::setprecision(2)
				<< bytes / static_cast<double>(1024*1024) << "mB";
		}else if(bytes >= 1024){
			oss << std::fixed << std::setprecision(1)
				<< bytes / static_cast<double>(1024) << "kB";
		}
		return oss.str();
	}

	/*
	Given two strings obtained from size_unit_select, returns less than (-1),
	equal to (0), or greater than (1). Just like strcmp.

	This function doesn't do any converting but it's related to size_SI so it
	goes in the convert namespace.
	*/
	static int size_SI_cmp(const std::string & left, const std::string & right)
	{
		//size prefix used for comparison
		const char * prefix = "Bkmgt";

		int left_prefix = -1;
		for(int x=0; x<left.size(); ++x){
			if(!((int)left[x] >= 48 && (int)left[x] <= 57) && left[x] != '.'){
				for(int y=0; y<5; ++y){
					if(left[x] == prefix[y]){
						left_prefix = y;
						goto end_left_find;
					}
				}
			}
		}
		end_left_find:

		int right_prefix = -1;
		for(int x=0; x<right.size(); ++x){
			if(!((int)right[x] >= 48 && (int)right[x] <= 57) && right[x] != '.'){
				for(int y=0; y<5; ++y){
					if(right[x] == prefix[y]){
						right_prefix = y;
						goto end_right_find;
					}
				}
			}
		}
		end_right_find:

		if(left_prefix == -1 || right_prefix == -1){
			return 0;
		}else if(left_prefix < right_prefix){
			return -1;
		}else if(right_prefix < left_prefix){
			return 1;
		}else{
			//same units on both sides, compare values
			std::string left_temp(left, 0, left.find(prefix[left_prefix]));
			std::string right_temp(right, 0, right.find(prefix[right_prefix]));
			left_temp.erase(std::remove(left_temp.begin(), left_temp.end(), '.'),
				left_temp.end());
			right_temp.erase(std::remove(right_temp.begin(), right_temp.end(), '.'),
				right_temp.end());
			return std::strcmp(left_temp.c_str(), right_temp.c_str());		
		}
	}

private:
	convert(){}

	//returns endianness of hardware
	enum { BIG_ENDIAN_ENUM, LITTLE_ENDIAN_ENUM };
	static int endianness()
	{
		union{
			boost::uint32_t num;
			unsigned char byte[sizeof(boost::uint32_t)];
		} test;
		test.num = 1;
		if(test.byte[0] == 1){
			return LITTLE_ENDIAN_ENUM;
		}else{
			return BIG_ENDIAN_ENUM;
		}
	}
};
#endif
