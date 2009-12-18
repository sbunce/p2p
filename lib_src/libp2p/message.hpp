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
	key_exchange_p_rA_t,
	key_exchange_rB_t,
	initial_t,
	error_t,
	request_slot_t,
	slot_t,
	request_hash_tree_block_t,
	request_file_block_t,
	block_t,
	have_hash_tree_block_t,
	have_file_block_t,
	close_slot_t,
	composite_t,
};

//abstract base class for all messages
class base
{
public:
	/*
	buf:
		Returns buffer to send, or buffer that was received.
	expects:
		Returns true if recv() expects message on front of CI.recv_buf.
	recv:
		Returns true if incoming message received.
	type:
		Returns the type of the message.
	*/
	virtual network::buffer & buf() = 0;
	virtual bool expects(network::connection_info & CI) = 0;
	virtual bool recv(network::connection_info & CI) = 0;
	virtual message::type type() = 0;
};

/*
Used in place of a std::pair to be more expressive.
Ex: MP.response instead of MP.second.
*/
class pair
{
public:
	pair();
	pair(
		boost::shared_ptr<base> request_in,
		boost::shared_ptr<base> response_in
	);
	boost::shared_ptr<base> request;
	boost::shared_ptr<base> response;
};

class key_exchange_p_rA : public base
{
public:
	//ctor to recv message
	key_exchange_p_rA();
	//ctor to send message
	explicit key_exchange_p_rA(encryption & Encryption);
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
};

class key_exchange_rB : public base
{
public:
	//ctor to recv message
	key_exchange_rB();
	//ctor to send message
	explicit key_exchange_rB(encryption & Encryption);
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
};

class initial : public base
{
public:
	//ctor to recv message
	initial();
	//ctor to send message
	explicit initial(const std::string peer_ID);
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
};

class request_slot : public base
{
public:
	//ctor to recv message
	request_slot();
	//ctor to send message
	explicit request_slot(const std::string & hash);
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
};

//the composite message is used to handle multiple possible responses
class composite : public base
{
public:
	//add messages to expect() and recv()
	void add(boost::shared_ptr<base> M);
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
	boost::shared_ptr<base> response;
	std::vector<boost::shared_ptr<base> > possible_response;
};

class slot : public base
{
public:
	//ctor to recv message
	explicit slot(const std::string & hash_in);
	//ctor to send message
	slot();
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
	//hash requested
	std::string hash;
	/*
	Set to true when file_size and root_hash checked. This is used to save
	processing from computing the hash multiple times.
	*/
	bool checked;
};

class error : public base
{
public:
	//ctor to recv and send (buf() always returns buffer with protocol::error)
	error();
	virtual network::buffer & buf();
	virtual bool expects(network::connection_info & CI);
	virtual bool recv(network::connection_info & CI);
	virtual message::type type();
private:
	network::buffer _buf;
};
}//end namespace message
#endif
