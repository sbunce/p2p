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
namespace {

//int -> big-endian binary
template<class T>
std::string int_to_bin(const T num)
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

/*
int -> bin_VLI
Note: VLI (variable length interger) is sized such that it can hold a range of
numbers [0, end)
Note: The VLI functions should only be used with unsigned ints.
*/
std::string int_to_bin_VLI(const boost::uint64_t num,
	const boost::uint64_t end)
{
	assert(num < end);
	//determine index of first used byte for max
	int index = 0;
	std::string temp = int_to_bin(end);
	for(; index<temp.size(); ++index){
		if(temp[index] != 0){
			break;
		}
	}
	//return bytes for value
	temp = int_to_bin(num);
	return temp.substr(index);
}

/*
big-endian binary -> int
Note: Only works with unsigned ints.
*/
template<class T>
T bin_to_int(const std::string encoded)
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

//bin_VLI -> integer
boost::uint64_t bin_VLI_to_int(std::string encoded)
{
	assert(encoded.size() > 0 && encoded.size() <= 8);
	//prepend zero bytes if needed
	encoded = std::string(8 - encoded.size(), '\0') + encoded;
	return bin_to_int<boost::uint64_t>(encoded);
}

//size of bin_VLI that int_to_bin_VLI would return
unsigned VLI_size(const boost::uint64_t end)
{
	assert(end > 0);
	unsigned index = 0;
	std::string temp = int_to_bin(end);
	for(; index<temp.size(); ++index){
		if(temp[index] != 0){
			return temp.size() - index;
		}
	}
	LOG; exit(1);
}

//returns true if string is valid hex
bool hex_validate(const std::string & hex)
{
	if(hex.size() == 0 || hex.size() % 2 != 0){
		return false;
	}
	for(int x=0; x<hex.size(); ++x){
		if(!(hex[x] >= '0' && hex[x] <= '9' || hex[x] >= 'A' && hex[x] <= 'F')){
			return false;
		}
	}
	return true;
}

/*
Returns binary version of hex string encoded in big-endian.
Precondition: Parameter must be valid hex string.
*/
std::string hex_to_bin(const std::string & hex)
{
	std::string bin;
	assert(hex_validate(hex));
	for(int x=0; x<hex.size(); x+=2){
		char ch = (hex[x] >= 'A' ? hex[x] - 'A' + 10 : hex[x] - '0') << 4;
		bin += ch + (hex[x+1] >= 'A' ? hex[x+1] - 'A' + 10 : hex[x+1] - '0');
	}
	return bin;
}

//converts a binary string to hex
std::string bin_to_hex(const std::string & bin)
{
	const char * const hex = "0123456789ABCDEF";
	std::string tmp;
	for(int x=0; x<bin.size(); ++x){
		tmp += hex[static_cast<int>((bin[x] >> 4) & 15)];
		tmp += hex[static_cast<int>(bin[x] & 15)];
	}
	return tmp;
}

//convert bytes to reasonable SI unit, example 1024 -> 1kB
std::string bytes_to_SI(const boost::uint64_t bytes)
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
equal to (0), or greater than (1).

Note: Assumes equal if size prefix not found in either left or right.

Note: This function only looks for the beginning of the size unit so it is ok to
pass it strings obtained from size_SI that have had additional characters
appended to them. For example it is ok to compare 5kB/s to 10kB/s.
*/
int SI_cmp(const std::string & left, const std::string & right)
{
	//size prefix used for comparison
	const char * prefix = "Bkmgt";

	//locate the size prefix
	int left_prefix = -1;
	for(int x=0; x<left.size(); ++x){
		if(!(left[x] >= 48 && left[x] <= 57) && left[x] != '.'){
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
		if(!(right[x] >= 48 && right[x] <= 57) && right[x] != '.'){
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
		//no prefix found, assume equal
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

//shortens string and adds ".." after it
std::string abbr(const std::string & str)
{
	if(str.size() < 6){
		return str;
	}else{
		return str.substr(0, 6) + "..";
	}
}
}//end of unnamed namespace
}//end of namespace convert
#endif
