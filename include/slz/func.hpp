#ifndef H_SLZ_FUNC
#define H_SLZ_FUNC

//include
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <logger.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <list>
#include <string>

namespace slz{
namespace {

//returns true if string is ASCII
bool is_ASCII(const std::string str)
{
	for(int x=0; x<str.size(); ++x){
		if(str[x] & 128){
			return false;
		}
	}
	return true;
}

/*
uint -> vint (variable length int)
Most sig bit of a byte is set to 1 if there is another byte to follow it. The
vint is little-endian (it has to be for vint encoding to work).
*/
std::string uint_to_vint(const boost::uint64_t x)
{
	union{
		boost::uint64_t num;
		char byte[sizeof(boost::uint64_t)];
	}num_byte = {x};
	#ifdef BOOST_BIG_ENDIAN
	std::reverse(num_byte.byte, num_byte.byte + sizeof(boost::uint64_t));
	#endif
	std::string vint(10, '\0');
	int end = 1;
	for(int x=0; x<vint.size(); ++x){
		vint[x] = num_byte.byte[0] & 127;
		if(vint[x]){
			end = x + 1;
		}
		vint[x] |= 128;
		num_byte.num >>= 7;
	}
	vint[end - 1] &= 127;
	return std::string(vint, 0, end);
}

//vint -> uint
boost::uint64_t vint_to_uint(const std::string & vint)
{
	assert(!vint.empty());
	assert(vint.size() <= 10);
	assert(!(vint[vint.size()-1] & 128));
	boost::uint64_t uint = 0;
	for(int x=vint.size()-1; x>=0; --x){
		uint <<= 7;
		uint |= vint[x] & 127;
	}
	return uint;
}

//return vint from front of buff, empty string if invalid vint
std::string vint_parse(const std::string & buf)
{
	if(buf.empty()){
		return "";
	}
	std::string vint;
	for(int x=0; x<10 && x<buf.size(); ++x){
		vint += buf[x];
		if(!(buf[x] & 128)){
			break;
		}
	}
	if(vint[vint.size()-1] & 128){
		return "";
	}else{
		return vint;
	}
}

/*
Return vint from front of buf, empty string if invalid vint.
Postcondition: If vint returned then vint removed from front of buf.
*/
std::string vint_split(std::string & buf)
{
	std::string vint = vint_parse(buf);
	buf.erase(0, vint.size());
	return vint;
}

//return field key
std::string key_create(boost::uint64_t field_UID, const bool length_delim)
{
	field_UID <<= 1;
	if(length_delim){
		field_UID |= 1;
	}
	return uint_to_vint(field_UID);
}

/*
Return key from front of buf, nothing if malformed field.
Precondition: Complete field must exist in front of buf.
*/
boost::optional<std::pair<boost::uint64_t, bool> > key_parse(const std::string & buf)
{
	std::string key_vint = vint_parse(buf);
	if(key_vint.empty()){
		return boost::optional<std::pair<boost::uint64_t, bool> >();
	}
	boost::uint64_t key = vint_to_uint(key_vint);
	bool length_delim = (key & 1);
	return std::make_pair(key >> 1, length_delim);
}

/*
Return key from front of buf, nothing if malformed field.
Precondition: Complete field must exist in front of buf.
Postcondition: If key returned then key removed from front of buf.
*/
boost::optional<std::pair<boost::uint64_t, bool> > key_split(std::string & buf)
{
	std::string key_vint = vint_split(buf);
	if(key_vint.empty()){
		return boost::optional<std::pair<boost::uint64_t, bool> >();
	}
	boost::uint64_t key = vint_to_uint(key_vint);
	bool length_delim = (key & 1);
	return std::make_pair(key >> 1, length_delim);
}

}//end namespace unnamed
}//end namespace slz
#endif
