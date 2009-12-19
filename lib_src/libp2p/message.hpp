#ifndef H_MESSAGE
#define H_MESSAGE

//custom
#include "encryption.hpp"
#include "protocol.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <network/network.hpp>

//standard
#include <list>
#include <map>

namespace message{

//message types (like RTTI and typeid())
enum type{
	block_t,
	close_slot_t,
	composite_t,
	error_t,
	have_hash_tree_block_t,
	have_file_block_t,
	initial_t,
	key_exchange_p_rA_t,
	key_exchange_rB_t,
	request_file_block_t,
	request_hash_tree_block_t,
	request_slot_t,
	slot_t,
};

//abstract base class for all messages
class base
{
public:
	//function to handle the message when it's received
	boost::function<bool (boost::shared_ptr<base>)> func;

	//bytes that have been sent or received
	network::buffer buf;

	/*
	expects:
		Returns true if recv() expects message on front of CI.recv_buf.
	recv:
		Returns true if incoming message received. False if incomplete message or
		host blacklisted.
	type:
		Returns the type of the message.
	*/
	virtual bool expects(network::connection_info & CI) = 0;
	virtual bool recv(network::connection_info & CI) = 0;
	virtual message::type type() = 0;
};

//the composite message is used to handle multiple possible responses
class composite : public base
{
public:
	//add messages to expect() and recv()
	void add(boost::shared_ptr<base> M);
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	boost::shared_ptr<base> response;
	std::vector<boost::shared_ptr<base> > possible_response;
};

class error : public base
{
public:
	//ctor to recv and send (buf() always returns buffer with protocol::error)
	error();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
};

class initial : public base
{
public:
	//ctor to recv message
	initial(boost::function<bool (boost::shared_ptr<base>)> func_in);
	//ctor to send message
	explicit initial(const std::string peer_ID);
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
};

class key_exchange_p_rA : public base
{
public:
	//ctor to recv message
	key_exchange_p_rA(boost::function<bool (boost::shared_ptr<base>)> func_in);
	//ctor to send message
	explicit key_exchange_p_rA(encryption & Encryption);
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
};

class key_exchange_rB : public base
{
public:
	//ctor to recv message
	key_exchange_rB(boost::function<bool (boost::shared_ptr<base>)> func_in);
	//ctor to send message
	explicit key_exchange_rB(encryption & Encryption);
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
};

class request_slot : public base
{
public:
	//ctor to recv message
	request_slot();
	//ctor to send message
	explicit request_slot(const std::string & hash);
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
};

class slot : public base
{
public:
	//ctor to recv message
	explicit slot(const std::string & hash_in);
	//ctor to send message
	slot();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	//hash requested
	std::string hash;
	/*
	Set to true when file_size and root_hash checked. This is used to save
	processing from computing the hash multiple times.
	*/
	bool checked;
};
}//end namespace message
#endif