#ifndef H_SLZ_MESSAGE
#define H_SLZ_MESSAGE

namespace slz{

//base class for user defined messages
template<boost::uint64_t T_field_UID>
class message : public field
{
public:
	static const boost::uint64_t field_UID = T_field_UID;
	static const bool length_delim = true;

	virtual operator bool () const
	{
		return !Field.empty() || !Unknown_Field.empty();
	}

	virtual void clear()
	{
		//erase unknown elements
		for(std::map<boost::uint64_t, boost::shared_ptr<field> >::iterator
			it_cur = Unknown_Field.begin(); it_cur != Unknown_Field.end();)
		{
			Field.erase(it_cur->first);
			Unknown_Field.erase(it_cur++);
		}
		//clear known elements
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			it_cur->second->clear();
		}
	}

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_parse(buf);
		if(!key || key->first != field_UID || key->second != true){
			return false;
		}
		std::list<std::string> fields = message_split(buf);
		if(fields.empty()){
			return false;
		}
		for(std::list<std::string>::iterator it_cur = fields.begin(),
			it_end = fields.end(); it_cur != it_end; ++it_cur)
		{
			key = key_parse(*it_cur);
			if(!key){
				return false;
			}
			std::map<boost::uint64_t, field *>::iterator it = Field.find(key->first);
			if(it == Field.end()){
				boost::shared_ptr<field> Unknown(new unknown());
				if(!Unknown->parse(*it_cur)){
					return false;
				}
				Field.insert(std::make_pair(Unknown->UID(), Unknown.get()));
				Unknown_Field.insert(std::make_pair(Unknown->UID(), Unknown));
			}else{
				if(!it->second->parse(*it_cur)){
					return false;
				}
			}
		}
		return true;
	}

	virtual std::string serialize() const
	{
		if(Field.empty()){
			return "";
		}
		std::string tmp;
		for(std::map<boost::uint64_t, field *>::const_iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			tmp += it_cur->second->serialize();
		}
		std::string ser;
		ser += key_create(field_UID, true);
		ser += uint_to_vint(tmp.size());
		ser += tmp;
		return ser;
	}

	virtual boost::uint64_t UID() const
	{
		return field_UID;
	}

protected:
	//must be called for all fields in derived
	void add_field(field & F)
	{
		Field.insert(std::make_pair(F.UID(), &F));
	}

private:
	//field ID mapped to field
	std::map<boost::uint64_t, field *> Field;

	//unknown fields, field pointer will also exist in Field container
	std::map<boost::uint64_t, boost::shared_ptr<field> > Unknown_Field;

	/*
	Return all fields in message, or empty list if malformed message.
	Postcondition: Parsed fields removed from buf. If message is malformed there
		are no guarantees on the state of buf after function returns.
	*/
	std::list<std::string> message_split(std::string & buf)
	{
		std::string key_vint = vint_split(buf);
		if(key_vint.empty() || buf.empty()){
			return std::list<std::string>();
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		bool key_length_delim = (key & 1);
		if(key_length_delim != true){
			return std::list<std::string>();
		}
		std::string m_size_vint = vint_split(buf);
		if(m_size_vint.empty() || buf.empty()){
			return std::list<std::string>();
		}
		boost::uint64_t m_size = vint_to_uint(m_size_vint);
		if(buf.size() != m_size){
			return std::list<std::string>();
		}
		std::list<std::string> tmp_fields;
		while(!buf.empty()){
			std::string key_vint = vint_split(buf);
			if(key_vint.empty() || buf.empty()){
				return std::list<std::string>();
			}
			boost::uint64_t key = vint_to_uint(key_vint);
			bool key_length_delim = (key & 1);
			if(key_length_delim){
				std::string f_size_vint = vint_split(buf);
				if(f_size_vint.empty() || buf.empty()){
					return std::list<std::string>();
				}
				boost::uint64_t f_size = vint_to_uint(f_size_vint);
				if(buf.size() < f_size){
					return std::list<std::string>();
				}
				tmp_fields.push_back(key_vint + f_size_vint + buf.substr(0, f_size));
				buf.erase(0, f_size);
			}else{
				std::string val_vint = vint_split(buf);
				if(val_vint.empty()){
					return std::list<std::string>();
				}
				tmp_fields.push_back(key_vint + val_vint);
			}
		}
		return tmp_fields;
	}
};

}//end of namespace slz
#endif
