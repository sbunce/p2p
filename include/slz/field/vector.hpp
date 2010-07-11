#ifndef H_SLZ_FIELD_VECTOR
#define H_SLZ_FIELD_VECTOR

namespace slz{

template<typename field_type>
class vector : public field
{
public:
	static const boost::uint64_t field_UID = field_type::field_UID;
	static const bool length_delim = true;

	vector()
	{

	}

	vector(const std::vector<field_type> & val_in):
		val(val_in)
	{

	}

	vector<field_type> & operator = (const std::vector<field_type> & rval)
	{
		val = rval;
		return *this;
	}

	std::vector<field_type> & operator * ()
	{
		return val;
	}

	std::vector<field_type> * operator -> ()
	{
		return &val;
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
		return field_type::field_UID;
	}

	virtual bool parse(std::string buf)
	{
		/*
		To parse we take the key and prepend it to every list element before we
		pass the list elements to be parsed by the field_type. This makes it so
		fields don't have to understand the "packed" format.
		*/
		clear();
		std::string key_vint = vint_parse(buf);
		buf.erase(0, key_vint.size());
		if(key_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t key_field_UID = (key >> 1);
		bool key_length_delim = (key & 1);
		if(key_field_UID != field_type::field_UID || key_length_delim != true){
			return false;
		}
		if(field_type::length_delim == false && key_length_delim == true){
			/*
			The vector is length delimited but the elements are not. Change the
			length delimited bit on the key before passing it to the field
			object to be parsed.
			*/
			key = (key >> 1) << 1;
			key_vint = uint_to_vint(key);
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
		while(!buf.empty()){
			std::string packed_field;
			if(field_type::length_delim){
				std::string f_size_vint = vint_parse(buf);
				if(f_size_vint.empty() || buf.empty()){
					return false;
				}
				buf.erase(0, f_size_vint.size());
				boost::uint64_t f_size = vint_to_uint(f_size_vint);
				if(buf.size() < f_size){
					return false;
				}
				std::string reassembled = key_vint + f_size_vint + buf.substr(0, f_size);
				buf.erase(0, f_size);
				field_type Field;
				if(!Field.parse(reassembled)){
					return false;
				}
				val.push_back(Field);
			}else{
				std::string val_vint = vint_parse(buf);
				if(val_vint.empty()){
					return false;
				}
				buf.erase(0, val_vint.size());
				std::string reassembled = key_vint + val_vint;
				field_type Field;
				if(!Field.parse(reassembled)){
					return false;
				}
				val.push_back(Field);
			}
		}
		return true;
	}

	virtual std::string serialize() const
	{
		/*
		Vector elements are "packed" together. Their keys are stripped. Only one
		key is left at the start of the list. Format of a "packed" list is:
		<key><field_size><val_size><val> + <val_size><val> + ...
		*/
		if(val.empty()){
			return "";
		}
		std::string packed;
		for(typename std::vector<field_type>::const_iterator it_cur = val.begin(),
			it_end = val.end(); it_cur != it_end; ++it_cur)
		{
			std::string tmp = it_cur->serialize();
			std::string key_vint = vint_parse(tmp);
			if(key_vint.empty()){
				continue;
			}else{
				tmp.erase(0, key_vint.size());
			}
			packed += tmp;
		}
		if(packed.empty()){
			return "";
		}
		boost::uint64_t key = (field_type::field_UID << 1);
		key |= 1;
		packed = uint_to_vint(key) + uint_to_vint(packed.size()) + packed;
		return packed;
	}

private:
	std::vector<field_type> val;
};

}//end namespace slz
#endif
