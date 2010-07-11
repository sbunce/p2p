#ifndef H_SLZ_FUNC
#define H_SLZ_FUNC

//include
#include <boost/cstdint.hpp>
#include <logger.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <list>
#include <string>

namespace slz{

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

//return all fields in message, or empty list if malformed message
std::list<std::string> message_to_fields(std::string buf)
{
	std::string m_size_vint = vint_parse(buf);
	if(m_size_vint.empty() || buf.empty()){
		return std::list<std::string>();
	}
	buf.erase(0, m_size_vint.size());
	boost::uint64_t m_size = vint_to_uint(m_size_vint);
	if(buf.size() != m_size){
		return std::list<std::string>();
	}
	std::list<std::string> tmp_fields;
	while(!buf.empty()){
		std::string key_vint = vint_parse(buf);
		if(key_vint.empty() || buf.empty()){
			return std::list<std::string>();
		}
		buf.erase(0, key_vint.size());
		boost::uint64_t key = vint_to_uint(key_vint);
		bool key_length_delim = (key & 1);
		if(key_length_delim){
			std::string f_size_vint = vint_parse(buf);
			if(f_size_vint.empty() || buf.empty()){
				return std::list<std::string>();
			}
			buf.erase(0, f_size_vint.size());
			boost::uint64_t f_size = vint_to_uint(f_size_vint);
			if(buf.size() < f_size){
				return std::list<std::string>();
			}
			tmp_fields.push_back(key_vint + f_size_vint + buf.substr(0, f_size));
			buf.erase(0, f_size);
		}else{
			std::string val_vint = vint_parse(buf);
			if(val_vint.empty()){
				return std::list<std::string>();
			}
			buf.erase(0, val_vint.size());
			tmp_fields.push_back(key_vint + val_vint);
		}
	}
	return tmp_fields;
}

//return all field_IDs in message, or empty list if malformed message
std::list<boost::uint64_t> message_to_field_IDs(const std::string & buf)
{
	std::list<std::string> fields = message_to_fields(buf);
	if(fields.empty()){
		return std::list<boost::uint64_t>();
	}
	std::list<boost::uint64_t> tmp;
	for(std::list<std::string>::iterator it_cur = fields.begin(),
		it_end = fields.end(); it_cur != it_end; ++it_cur)
	{
		std::string key_vint = vint_parse(*it_cur);
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t field_UID = (key >> 1);
		tmp.push_back(field_UID);
	}
	return tmp;
}

}//end of namespace slz
#endif
