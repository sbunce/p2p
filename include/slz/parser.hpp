#ifndef H_SLZ_PARSER
#define H_SLZ_PARSER

//custom
#include "field.hpp"

namespace slz{

//parser for incoming fields
class parser
{
public:
	parser():
		bad(false)
	{}

	/*
	Add field parser will understand. T must be a field.
	Precondition: This must not have been called for T before.
	Postcondition: Field cleared.
	*/
	template<typename T>
	void add_field()
	{
		boost::shared_ptr<T> new_field(new T());
		std::pair<std::map<boost::uint64_t, boost::shared_ptr<field> >::iterator, bool>
			p = Field.insert(std::make_pair(new_field->UID(), new_field));
		if(!p.second){
			LOG << "error, field UID " << new_field->UID() << " not unique";
			exit(1);
		}
	}

	//returns true if field in buf was malformed
	bool bad_stream() const
	{
		return bad;
	}

	/*
	Parse fields in buf. Fields must have length prefixed. Returns empty
	shared_ptr if incomplete field in buf.
	Postcondition: Parsed field removed from buf.
	*/
	boost::shared_ptr<field> parse(std::string & buf)
	{
		std::string f_buf = field_split(buf);
		if(f_buf.empty()){
			return boost::shared_ptr<field>();
		}
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_parse(f_buf);
		if(!key){
			bad = true;
			return boost::shared_ptr<field>();
		}
		std::map<boost::uint64_t, boost::shared_ptr<field> >::iterator
			it = Field.find(key->first);
		if(it == Field.end()){
			boost::shared_ptr<field> Unknown(new unknown());
			if(Unknown->parse(f_buf)){
				return Unknown;
			}else{
				return boost::shared_ptr<field>();
			}
		}else{
			if(it->second->parse(f_buf)){
				return it->second;
			}else{
				bad = true;
				return boost::shared_ptr<field>();
			}
		}
	}

private:
	//true if bad field passed to parse
	bool bad;

	//maps field UID to field it belongs to
	std::map<boost::uint64_t, boost::shared_ptr<field> > Field;

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

}//end of namespace slz
#endif
