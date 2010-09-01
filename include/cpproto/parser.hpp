#ifndef H_CPPROTO_PARSER
#define H_CPPROTO_PARSER

//custom
#include "field/unknown.hpp"

//include
#include <boost/shared_ptr.hpp>

namespace cpproto{

//parser for incoming fields
class parser
{
public:
	parser():
		_bad(false)
	{}

	//register call back to be used when field is parsed
	template<typename T>
	void reg_handler(boost::function<void (T &)> func)
	{
		boost::shared_ptr<T> new_field(new T());
		std::pair<std::map<boost::uint64_t, field_element>::iterator, bool>
			p = Field.insert(std::make_pair(new_field->ID(), field_element(
			new_field, boost::bind(func, boost::ref(*new_field)))));
		if(!p.second){
			LOG << "non-unique field ID " << new_field->ID();
			exit(1);
		}
	}

	//register call back for unknown fields, otherwise they're ignored
	void reg_unknown_handler(boost::function<void (unknown &)> unknown_call_back_in)
	{
		if(unknown_call_back){
			LOG << "tried to register unknown handler twice";
			exit(1);
		}
		unknown_call_back = unknown_call_back_in;
	}

	//returns true if buf passed to parse was malformed
	bool bad() const
	{
		return _bad;
	}

	/*
	Parse fields in buf.
	Postcondition: Parsed field removed from buf.
	*/
	void parse(std::string & buf)
	{
		while(true){
			std::string f_buf = field_split(buf);
			if(f_buf.empty()){
				break;
			}
			boost::optional<std::pair<boost::uint64_t, bool> > key = key_parse(f_buf);
			if(!key){
				_bad = true;
				break;
			}
			std::map<boost::uint64_t, field_element>::iterator
				it = Field.find(key->first);
			if(it == Field.end()){
				if(unknown_call_back){
					unknown Unknown;
					if(Unknown.parse(f_buf)){
						unknown_call_back(Unknown);
					}else{
						break;
					}
				}
			}else{
				if(it->second.Field->parse(f_buf)){
					it->second.call_back();
				}else{
					_bad = true;
					break;
				}
			}
		}
	}

private:
	//true if bad field passed to parse
	bool _bad;

	class field_element
	{
	public:
		field_element(
			const boost::shared_ptr<field> & Field_in,
			const boost::function<void ()> & call_back_in
		):
			Field(Field_in),
			call_back(call_back_in)
		{}
		boost::shared_ptr<field> Field;
		boost::function<void ()> call_back;
	};

	//maps field ID to field
	std::map<boost::uint64_t, field_element> Field;

	//call back for unknown fields (used if not empty)
	boost::function<void (unknown &)> unknown_call_back;

	/*
	Return field from front of buf, empty string if incomplete field.
	Postcondition: If field returned then field removed from front of buf.
	*/
	std::string field_split(std::string & buf)
	{
		std::string key_vint = vint_parse(buf);
		if(key_vint.empty() || key_vint.size() == buf.size()){
			return "";
		}
		boost::optional<std::pair<boost::uint64_t, bool> >
			key = key_parse(key_vint);
		if(!key){
			return "";
		}
		std::string tmp = buf.substr(key_vint.size(), 10);
		if(key->second){
			std::string len_vint = vint_parse(tmp);
			boost::uint64_t len = vint_to_uint(len_vint);
			if(buf.size() < key_vint.size() + len_vint.size() + len){
				return "";
			}
			tmp = buf.substr(0, key_vint.size() + len_vint.size() + len);
			buf.erase(0, key_vint.size() + len_vint.size() + len);
		}else{
			std::string val_vint = vint_parse(tmp);
			if(val_vint.empty()){
				return "";
			}
			tmp = buf.substr(0, key_vint.size() + val_vint.size());
			buf.erase(0, key_vint.size() + val_vint.size());
		}
		return tmp;
	}
};

}//end of namespace cpproto
#endif
