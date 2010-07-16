#ifndef H_SLZ_FIELD_ASCII
#define H_SLZ_FIELD_ASCII

namespace slz{

/*
String field which serializes to 7-bit ASCII to save space.
A 8 byte ASCII string would be 7 bytes serialized.
*/
template<boost::uint64_t T_field_ID>
class ASCII : public field
{
public:
	static const boost::uint64_t field_ID = T_field_ID;
	static const bool length_delim = true;

	ASCII()
	{

	}

	ASCII(const std::string & val_in):
		val(val_in)
	{

	}

	ASCII(const char * val_in):
		val(val_in)
	{

	}

	ASCII & operator = (const std::string & rval)
	{
		val = rval;
		return *this;
	}

	std::string & operator * ()
	{
		return val;
	}

	std::string & operator -> ()
	{
		return val;
	}

	virtual operator bool () const
	{
		return !val.empty();
	}

	virtual void clear()
	{
		val.clear();
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_split(buf);
		if(!key || key->first != field_ID || key->second != true){
			return false;
		}
		std::string len_vint = vint_split(buf);
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
		if(val.empty() || !is_ASCII(val)){
			return "";
		}
		std::string enc = ASCII_encode(val);
		std::string ser;
		ser += key_create(field_ID, true);
		ser += uint_to_vint(enc.size());
		ser += enc;
		return ser;
	}

private:
	//stores unencoded string
	std::string val;

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

}//end namespace slz
#endif
