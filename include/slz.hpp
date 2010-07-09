#ifndef H_SLZ
#define H_SLZ

//include
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.hpp>

//standard
#include <cassert>
#include <limits>
#include <list>
#include <map>
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
		bool len_delim = (key & 1);
		if(len_delim){
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
		boost::uint64_t field_ID = (key >> 1);
		tmp.push_back(field_ID);
	}
	return tmp;
}

//base class for all fields
class field
{
public:
	/*
	operator bool:
		True if field set (acts like boost::optional).
	clear:
		Unset field (if set).
	parse:
		Parse field. Return false if field malformed. Field cleared before parse.
	serialize:
		Returns serialized version of field.
	*/
	virtual operator bool () const = 0;
	virtual void clear() = 0;
	virtual boost::uint64_t ID() const = 0;
	virtual bool parse(std::string buf) = 0;
	virtual std::string serialize() const = 0;
};

//7-bit ASCII encoded variable length string (max_val_size is 7 bit groups)
template<boost::uint64_t field_ID>
class ASCII : public field
{
public:
	ASCII & operator = (const std::string & rval)
	{
		val = rval;
		return *this;
	}

	std::string & operator * ()
	{
		return *val;
	}

	boost::optional<std::string> & operator -> ()
	{
		return *val;
	}

	virtual operator bool () const
	{
		return val;
	}

	virtual void clear()
	{
		if(val){
			val.reset();
		}
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		std::string key_vint = vint_parse(buf);
		buf.erase(0, key_vint.size());
		if(key_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t key_field_ID = (key >> 1);
		bool len_delim = (key & 1);
		if(key_field_ID != field_ID || len_delim != true){
			return false;
		}
		std::string len_vint = vint_parse(buf);
		buf.erase(0, len_vint.size());
		if(len_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t len = vint_to_uint(len_vint);
		if(buf.size() != len){
			return false;
		}
		val = ASCII_decode(buf);
		return true;
	}

	virtual std::string serialize() const
	{
		if(!val || val->empty() || !is_ASCII(*val)){
			return "";
		}
		boost::uint64_t key = (field_ID << 1);
		key |= 1;
		std::string tmp;
		tmp += uint_to_vint(key);
		std::string enc = ASCII_encode(*val);
		tmp += uint_to_vint(enc.size()) + enc;
		return tmp;
	}

private:
	//stores unencoded string
	boost::optional<std::string> val;

	//ASCII stored in 8 bits -> ASCII stored in 7 bits
	std::string ASCII_encode(const std::string & str) const
	{
		int res_size = str.size() * 7 % 8 == 0 ?
			str.size() * 7 / 8 : str.size() * 7 / 8 + 1;
		std::string res(res_size,'\0');
		for(int x=0; x<str.size(); ++x){
			int byte_offset = x * 7 / 8;
			int bit_offset = x * 7 % 8;
			char tmp;
			//write bits in first byte
			tmp = str[x] & 127;
			tmp <<= bit_offset;
			res[byte_offset] |= tmp;
			//write bits in second byte
			if(bit_offset + 7 > 8){
				//write bits in second byte
				tmp = str[x] & 127;
				tmp >>= -bit_offset + 8;
				res[byte_offset+1] |= tmp;
			}
		}
		return res;
	}

	//ASCII stored in 7 bits -> ASCII stored in 8 bits
	std::string ASCII_decode(const std::string & str) const
	{
		int res_size = str.size() * 8 % 7 == 0 ?
			str.size() * 8 / 7 : str.size() * 8 / 7 + 1;
		std::string res(res_size, '\0');
		for(int x=0; x<res_size; ++x){
			int byte_offset = x * 7 / 8;
			int bit_offset = x * 7 % 8;
			unsigned char tmp, combine = 0;
			if(bit_offset + 7 < 8){
				//bits on left don't belong to this 7-char
				tmp = str[byte_offset];
				tmp <<= 8 - bit_offset - 7;
				tmp >>= 1;
				combine |= tmp;
			}else{
				//bits on left all belong to this 7-char
				tmp = str[byte_offset];
				tmp >>= bit_offset;
				combine |= tmp;
				if(bit_offset + 7 != 8){
					//7-char spans two bytes
					tmp = str[byte_offset+1];
					tmp <<= 16 - bit_offset - 7;
					tmp >>= 16 - bit_offset - 7;
					tmp <<= -bit_offset + 8;
					combine |= tmp;
				}
			}
			res[x] = static_cast<char>(combine);
		}
		if(res[res.size()-1] == '\0'){
			//last byte is padding
			res.resize(res.size()-1);
		}
		return res;
	}
};

//signed integer
template<boost::uint64_t field_ID>
class sint : public field
{
public:
	sint & operator = (const boost::int64_t rval)
	{
		val = rval;
		return *this;
	}

	boost::int64_t & operator * ()
	{
		return *val;
	}

	virtual operator bool () const
	{
		return val;
	}

	virtual void clear()
	{
		if(val){
			val.reset();
		}
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		std::string key_vint = vint_parse(buf);
		buf.erase(0, key_vint.size());
		if(key_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t key_field_ID = (key >> 1);
		bool len_delim = (key & 1);
		if(key_field_ID != field_ID || len_delim != false){
			return false;
		}
		std::string val_vint = vint_parse(buf);
		buf.erase(0, val_vint.size());
		if(val_vint.empty() || !buf.empty()){
			return false;
		}
		val = decode_sint(vint_to_uint(val_vint));
		return true;
	}

	virtual std::string serialize() const
	{
		if(!val){
			return "";
		}
		boost::uint64_t key = (field_ID << 1);
		std::string tmp;
		tmp += uint_to_vint(key);
		tmp += uint_to_vint(encode_sint(*val));
		return tmp;
	}

private:
	boost::optional<boost::int64_t> val;

	//encode signed as unsigned, needed to store signed int as vint
	boost::uint64_t encode_sint(const boost::int64_t x) const
	{
		return (x << 1) ^ (x >> 63);
	}

	//decode signed int encoded as unsigned
	boost::int64_t decode_sint(const boost::uint64_t x) const
	{
		return (x >> 1) ^ -static_cast<boost::int64_t>(x & 1);
	}
};

//variable length string (max_val_size is bytes)
template<boost::uint64_t field_ID>
class string : public field
{
public:
	string & operator = (const std::string & rval)
	{
		val = rval;
		return *this;
	}

	std::string & operator * ()
	{
		return *val;
	}

	boost::optional<std::string> & operator -> ()
	{
		return *val;
	}

	virtual operator bool () const
	{
		return val;
	}

	virtual void clear()
	{
		if(val){
			val.reset();
		}
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		std::string key_vint = vint_parse(buf);
		buf.erase(0, key_vint.size());
		if(key_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t key_field_ID = (key >> 1);
		bool len_delim = (key & 1);
		if(key_field_ID != field_ID || len_delim != true){
			return false;
		}
		std::string len_vint = vint_parse(buf);
		buf.erase(0, len_vint.size());
		if(len_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t len = vint_to_uint(len_vint);
		if(buf.size() != len){
			return false;
		}
		val = buf;
		return true;
	}

	virtual std::string serialize() const
	{
		if(!val || val->empty()){
			return "";
		}
		boost::uint64_t key = (field_ID << 1);
		key |= 1;
		std::string tmp;
		tmp += uint_to_vint(key);
		tmp += uint_to_vint(val->size()) + *val;
		return tmp;
	}

private:
	boost::optional<std::string> val;
};

//unsigned integer
template<boost::uint64_t field_ID>
class uint : public field
{
public:
	uint & operator = (const boost::uint64_t rval)
	{
		val = rval;
		return *this;
	}

	boost::uint64_t & operator * ()
	{
		return *val;
	}

	virtual operator bool () const
	{
		return val;
	}

	virtual void clear()
	{
		if(val){
			val.reset();
		}
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		std::string key_vint = vint_parse(buf);
		buf.erase(0, key_vint.size());
		if(key_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t key_field_ID = (key >> 1);
		bool len_delim = (key & 1);
		if(key_field_ID != field_ID || len_delim != false){
			return false;
		}
		std::string val_vint = vint_parse(buf);
		buf.erase(0, val_vint.size());
		if(val_vint.empty() || !buf.empty()){
			return false;
		}
		val = vint_to_uint(val_vint);
		return true;
	}

	virtual std::string serialize() const
	{
		if(!val){
			return "";
		}
		boost::uint64_t key = (field_ID << 1);
		std::string tmp;
		tmp += uint_to_vint(key);
		tmp += uint_to_vint(*val);
		return tmp;
	}

private:
	boost::optional<boost::uint64_t> val;
};

//base class for user defined messages
class message
{
public:
	/*
	This virtual dtor makes the message type polymorphic. This is required so
	that typeid returns the derived type instead of the base type.
	*/
	virtual ~message(){}

	//clear all fields
	void clear()
	{
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			it_cur->second->clear();
		}
	}

	//returns all field IDs in this message
	std::list<boost::uint64_t> ID()
	{
		std::list<boost::uint64_t> tmp;
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			tmp.push_back(it_cur->first);
		}
		return tmp;
	}

	/*
	Parse message. Returns false if field malformed. Fields cleared before
	parsing attempted.
	*/
	bool parse(const std::string & buf)
	{
		clear();
		std::list<std::string> fields = message_to_fields(buf);
		if(fields.empty()){
			return false;
		}
		for(std::list<std::string>::iterator it_cur = fields.begin(),
			it_end = fields.end(); it_cur != it_end; ++it_cur)
		{
			std::string key_vint = vint_parse(*it_cur);
			if(key_vint.empty() || buf.empty()){
				return false;
			}
			boost::uint64_t key = vint_to_uint(key_vint);
			boost::uint64_t field_ID = (key >> 1);
			std::map<boost::uint64_t, field *>::iterator it = Field.find(field_ID);
			if(it != Field.end()){
				if(!it->second->parse(*it_cur)){
					return false;
				}
			}
		}
		return true;
	}

	//returns serialized version of all fields
	std::string serialize()
	{
		std::string tmp;
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			tmp += it_cur->second->serialize();
		}
		tmp = uint_to_vint(tmp.size()) + tmp;
		return tmp;
	}

protected:
	//must be called for all fields
	void add_field(field & F)
	{
		Field.insert(std::make_pair(F.ID(), &F));
	}

private:
	//field ID mapped to field
	std::map<boost::uint64_t, field *> Field;
};

//parser for incoming messages
class parser
{
public:
	parser():
		_good(true)
	{}

	/*
	Add message parser will understand.
	Postcondition: Message cleared.
	*/
	void add_message(const boost::shared_ptr<message> & M)
	{
		assert(M);
		std::list<boost::uint64_t> ID = M->ID();
		for(std::list<boost::uint64_t>::iterator it_cur = ID.begin(),
			it_end = ID.end(); it_cur != it_end; ++it_cur)
		{
			std::pair<std::map<boost::uint64_t, boost::shared_ptr<message> >::iterator, bool>
				p = Message.insert(std::make_pair(*it_cur, M));
			if(!p.second){
				LOG << "error, field ID " << *it_cur << " not unique";
				exit(1);
			}
		}
	}

	//returns true if message in buf was malformed
	bool good() const
	{
		return _good;
	}

	/*
	Parse messages in buf. Messages must have length prefixed. Returns empty
	shared_ptr if incomplete message in buf.
	Postcondition: Parsed message removed from buf.
	*/
	boost::shared_ptr<message> parse(std::string & buf)
	{
		std::string m_size_vint = vint_parse(buf);
		if(m_size_vint.empty()){
			return boost::shared_ptr<message>();
		}
		boost::uint64_t m_size = vint_to_uint(m_size_vint);
		if(buf.size() < m_size_vint.size() + m_size){
			return boost::shared_ptr<message>();
		}
		std::string m_buf = buf.substr(0, m_size_vint.size() + m_size);
		buf.erase(0, m_size_vint.size() + m_size);
		std::list<boost::uint64_t> field_IDs = message_to_field_IDs(m_buf);
		if(field_IDs.empty()){
			_good = false;
			return boost::shared_ptr<message>();
		}
		//all field_IDs must be from same parent message
		boost::shared_ptr<message> found_parent;
		for(std::list<boost::uint64_t>::iterator it_cur = field_IDs.begin(),
			it_end = field_IDs.end(); it_cur != it_end; ++it_cur)
		{
			std::map<boost::uint64_t, boost::shared_ptr<message> >::iterator
				it = Message.find(*it_cur);
			if(it != Message.end()){
				if(found_parent){
					if(found_parent != it->second){
						_good = false;
						return boost::shared_ptr<message>();
					}
				}else{
					found_parent = it->second;
				}
			}
		}
		if(found_parent){
			if(found_parent->parse(m_buf)){
				return found_parent;
			}else{
				_good = false;
				return boost::shared_ptr<message>();
			}
		}else{
			return boost::shared_ptr<message>();
		}
	}

private:
	//true if bad message passed to parse
	bool _good;

	//maps field ID to message it belongs to
	std::map<boost::uint64_t, boost::shared_ptr<message> > Message;
};

}//end namespace serialize
#endif
