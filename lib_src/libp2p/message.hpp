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

//message types (same function as RTTI and typeid())
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
	pair(){}
	pair(
		boost::shared_ptr<base> request_in,
		boost::shared_ptr<base> response_in
	):
		request(request_in),
		response(response_in)
	{}
	boost::shared_ptr<base> request;
	boost::shared_ptr<base> response;
};

class key_exchange_p_rA : public base
{
public:
	//ctor to recv message
	key_exchange_p_rA(){}

	//ctor to send message
	explicit key_exchange_p_rA(encryption & Encryption)
	{
		_buf = Encryption.send_p_rA();
	}

	virtual network::buffer & buf()
	{
		return _buf;
	}
	virtual bool expects(network::connection_info & CI)
	{
		//p rA doesn't begin with any command
		return true;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE * 2){
			_buf.append(CI.recv_buf.data(), protocol::DH_KEY_SIZE * 2);
			CI.recv_buf.erase(0, protocol::DH_KEY_SIZE * 2);
			return true;
		}
		return false;
	}
	virtual message::type type(){ return key_exchange_p_rA_t; }
private:
	network::buffer _buf;
};

class key_exchange_rB : public base
{
public:
	//ctor to recv message
	key_exchange_rB(){}

	//ctor to send message
	explicit key_exchange_rB(encryption & Encryption)
	{
		_buf = Encryption.send_rB();
	}

	virtual network::buffer & buf()
	{
		return _buf;
	}
	virtual bool expects(network::connection_info & CI)
	{
		//rB doesn't begin with any command
		return true;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE){
			_buf.append(CI.recv_buf.data(), protocol::DH_KEY_SIZE);
			CI.recv_buf.erase(0, protocol::DH_KEY_SIZE);
			return true;
		}
		return false;
	}
	virtual message::type type(){ return key_exchange_rB_t; }
private:
	network::buffer _buf;
};

class initial : public base
{
public:
	//ctor to recv message
	initial(){}

	//ctor to send message
	explicit initial(const std::string peer_ID)
	{
		_buf = convert::hex_to_bin(peer_ID);
	}

	virtual network::buffer & buf()
	{
		return _buf;
	}
	virtual bool expects(network::connection_info & CI)
	{
		//initial doesn't begin with any command
		return true;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		if(CI.recv_buf.size() >= SHA1::bin_size){
			_buf.append(CI.recv_buf.data(), SHA1::bin_size);
			CI.recv_buf.erase(0, SHA1::bin_size);
			return true;
		}
		return false;
	}
	virtual message::type type(){ return initial_t; }
private:
	network::buffer _buf;
};

class request_slot : public base
{
public:
	//ctor to recv message
	request_slot(){}

	//ctor to send message
	explicit request_slot(const std::string & hash)
	{
		_buf.append(protocol::REQUEST_SLOT).append(convert::hex_to_bin(hash));
	}

	virtual network::buffer & buf()
	{
		return _buf;
	}
	virtual bool expects(network::connection_info & CI)
	{
		return CI.recv_buf[0] == protocol::REQUEST_SLOT;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		if(CI.recv_buf.size() >= protocol::REQUEST_SLOT_SIZE){
			_buf.append(CI.recv_buf.data(), protocol::REQUEST_SLOT_SIZE);
			CI.recv_buf.erase(0, protocol::REQUEST_SLOT_SIZE);
			return true;
		}
		return false;
	}
	virtual message::type type(){ return request_slot_t; }
private:
	network::buffer _buf;
};

//the composite message is used to handle multiple possible responses
class composite : public base
{
public:
	//add messages to expect() and recv()
	void add(boost::shared_ptr<base> M)
	{
		assert(M);
		possible_response.push_back(M);
	}
	virtual network::buffer & buf()
	{
		if(response){
			return response->buf();
		}else{
			return _buf;
		}
	}
	virtual bool expects(network::connection_info & CI)
	{
		for(std::vector<boost::shared_ptr<base> >::iterator
			iter_cur = possible_response.begin(), iter_end = possible_response.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if((*iter_cur)->expects(CI)){
				return true;
			}
		}
		return false;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		for(std::vector<boost::shared_ptr<base> >::iterator
			iter_cur = possible_response.begin(), iter_end = possible_response.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if((*iter_cur)->expects(CI)){
				if((*iter_cur)->recv(CI)){
					response = *iter_cur;
					return true;
				}
				return false;
			}
		}
	}
	virtual message::type type()
	{
		if(response){
			response->type();
		}else{
			return composite_t;
		}
	}
private:
	network::buffer _buf;
	boost::shared_ptr<base> response;
	std::vector<boost::shared_ptr<base> > possible_response;
};

class slot : public base
{
public:
	//ctor to recv message
	explicit slot(const std::string & hash_in):
		hash(hash_in),
		checked(false)
	{}

	//ctor to send message
	slot()
	{
//DEBUG, need to complete this
		//_buf.append(
	}

	virtual network::buffer & buf()
	{
		return _buf;
	}
	virtual bool expects(network::connection_info & CI)
	{
		return CI.recv_buf[0] == protocol::SLOT;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		if(CI.recv_buf.size() < protocol::SLOT_SIZE(0, 0)){
			//we do not have the minimum size slot message
			return false;
		}
		if(!checked){
			SHA1 SHA;
			SHA.init();
			SHA.load(reinterpret_cast<const char *>(CI.recv_buf.data()) + 3,
				8 + SHA1::bin_size);
			SHA.end();
			if(SHA.hex() != hash){
				LOGGER << "invalid slot message from " << CI.IP;
				database::table::blacklist::add(CI.IP);
				return false;
			}
		}
		if(CI.recv_buf[2] == static_cast<unsigned char>(0)){
			//no bitfields to follow
			_buf.append(CI.recv_buf.data(), protocol::SLOT_SIZE(0, 0));
			CI.recv_buf.erase(0, protocol::SLOT_SIZE(0, 0));
			return true;
		}else{
			LOGGER << "stub: need to process slot message where k != 0";
			exit(1);
		}
		return true;
	}
	virtual message::type type()
	{
		return message::slot_t;
	}
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
	//ctor to recv and send (buf() always returns buffer with protocol::ERROR)
	error()
	{
		_buf.append(protocol::ERROR);
	}

	virtual network::buffer & buf()
	{
		return _buf;
	}
	virtual bool expects(network::connection_info & CI)
	{
		return CI.recv_buf[0] == protocol::ERROR;
	}
	virtual bool recv(network::connection_info & CI)
	{
		assert(expects(CI));
		if(CI.recv_buf[0] == protocol::ERROR){
			CI.recv_buf.erase(0, protocol::ERROR_SIZE);
			return true;
		}
		return false;
	}
	virtual message::type type(){ return error_t; }
private:
	network::buffer _buf;
};
}//end namespace message
#endif
