#ifndef H_CONVERT
#define H_CONVERT

//include
#include <boost/cstdint.hpp>
#include <boost/detail/endian.hpp>
#include <logger.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

namespace convert {

//returns num encoded in big-endian
template<class T>
static std::string encode(const T & num)
{
	union{
		T n;
		char b[sizeof(T)];
	} NB;
	NB.n = num;
	#ifdef BOOST_LITTLE_ENDIAN
	std::reverse(NB.b, NB.b+sizeof(T));
	#endif
	return std::string(NB.b, sizeof(T));
}

//decode number encoded in big-endian
template<class T>
static T decode(const std::string & encoded)
{
	assert(encoded.size() == sizeof(T));
	union{
		T n;
		char b[sizeof(T)];
	} NB;
	#ifdef BOOST_LITTLE_ENDIAN
	std::reverse_copy(encoded.data(), encoded.data()+sizeof(T), NB.b);
	#elif
	std::copy(encoded.data(), encoded.data()+sizeof(T), NB.b);
	#endif
	return NB.n;
}

/*
Returns binary version of hex string encoded in big-endian. Returns empty string
if invalid hex string.
Examples of Invalid:
	""       //empty
	"123ABG" //'G' not in hex
	"123"    //string_size % 2 != 0
*/
static std::string hex_to_bin(std::string hex)
{
	std::string bin;
	if(hex.empty() || hex.size() % 2 != 0){
		return bin;
	}
	for(int x=0; x<hex.size(); x+=2){
		hex[x] = std::toupper(hex[x]);
		hex[x+1] = std::toupper(hex[x+1]);
		if(!(hex[x] >= '0' && hex[x] <= '9' || hex[x] >= 'A' && hex[x] <= 'F')
			|| !(hex[x+1] >= '0' && hex[x+1] <= '9' || hex[x+1] >= 'A' && hex[x+1] <= 'F'))
		{
			bin.clear();
			return bin;
		}
		char ch;
		ch = (hex[x] >= 'A' ? hex[x] - 'A' + 10 : hex[x] - '0') << 4;
		bin += ch + (hex[x+1] >= 'A' ? hex[x+1] - 'A' + 10 : hex[x+1] - '0');
	}
	return bin;
}

//converts a binary string to hex
static std::string bin_to_hex(const std::string & bin)
{
	const char * const HEX = "0123456789ABCDEF";
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

Note: This function only looks for the beginning of the size unit so it is ok to
pass it strings obtained from size_SI that have had additional characters
appended to them. For example it is ok to compare 5kB/s to 10kB/s.
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
}//end of namespace convert
#endif
