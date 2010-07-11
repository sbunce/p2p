#ifndef H_SLZ_PARSER
#define H_SLZ_PARSER

//custom
#include "message.hpp"

namespace slz{

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

}//end of namespace slz
#endif
