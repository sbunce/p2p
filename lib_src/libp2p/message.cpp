#include "message.hpp"

//BEGIN base
bool message::base::encrypt()
{
	return true;
}

//END base

//BEGIN composite
void message::composite::add(boost::shared_ptr<base> M)
{
	assert(M);
	possible_response.push_back(M);
}

bool message::composite::expects(network::connection_info & CI)
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

bool message::composite::recv(network::connection_info & CI)
{
	assert(expects(CI));
	for(std::vector<boost::shared_ptr<base> >::iterator
		iter_cur = possible_response.begin(), iter_end = possible_response.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->expects(CI)){
			if((*iter_cur)->recv(CI)){
				func = (*iter_cur)->func;
				buf.move((*iter_cur)->buf);
				response = *iter_cur;
				return true;
			}
			return false;
		}
	}
}
//END composite

//BEGIN error
message::error::error(boost::function<bool (boost::shared_ptr<base>)> func_in)
{
	func = func_in;
}

message::error::error()
{
	buf.append(protocol::error);
}

bool message::error::expects(network::connection_info & CI)
{
	return CI.recv_buf[0] == protocol::error;
}

bool message::error::recv(network::connection_info & CI)
{
	assert(expects(CI));
	if(CI.recv_buf[0] == protocol::error){
		buf.append(CI.recv_buf.data(), protocol::error_size);
		CI.recv_buf.erase(0, protocol::error_size);
		return true;
	}
	return false;
}
//END error

//BEGIN initial
message::initial::initial(boost::function<bool (boost::shared_ptr<base>)> func_in)
{
	func = func_in;
}

message::initial::initial(const std::string peer_ID)
{
	buf = convert::hex_to_bin(peer_ID);
}

bool message::initial::expects(network::connection_info & CI)
{
	//initial doesn't begin with any command
	return true;
}

bool message::initial::recv(network::connection_info & CI)
{
	assert(expects(CI));
	if(CI.recv_buf.size() >= SHA1::bin_size){
		buf.append(CI.recv_buf.data(), SHA1::bin_size);
		CI.recv_buf.erase(0, SHA1::bin_size);
		return true;
	}
	return false;
}
//END initial

//BEGIN key_exchange_p_rA
message::key_exchange_p_rA::key_exchange_p_rA(
	boost::function<bool (boost::shared_ptr<base>)> func_in)
{
	func = func_in;
}

message::key_exchange_p_rA::key_exchange_p_rA(encryption & Encryption)
{
	buf = Encryption.send_p_rA();
}

bool message::key_exchange_p_rA::encrypt()
{
	return false;
}

bool message::key_exchange_p_rA::expects(network::connection_info & CI)
{
	//p rA doesn't begin with any command
	return true;
}

bool message::key_exchange_p_rA::recv(network::connection_info & CI)
{
	assert(expects(CI));
	if(CI.recv_buf.size() >= protocol::DH_key_size * 2){
		buf.append(CI.recv_buf.data(), protocol::DH_key_size * 2);
		CI.recv_buf.erase(0, protocol::DH_key_size * 2);
		return true;
	}
	return false;
}
//END key_exchange_p_rA

//BEGIN key_exchange_rB
message::key_exchange_rB::key_exchange_rB(
	boost::function<bool (boost::shared_ptr<base>)> func_in)
{
	func = func_in;
}

message::key_exchange_rB::key_exchange_rB(encryption & Encryption)
{
	buf = Encryption.send_rB();
}

bool message::key_exchange_rB::encrypt()
{
	return false;
}

bool message::key_exchange_rB::expects(network::connection_info & CI)
{
	//rB doesn't begin with any command
	return true;
}

bool message::key_exchange_rB::recv(network::connection_info & CI)
{
	assert(expects(CI));
	if(CI.recv_buf.size() >= protocol::DH_key_size){
		buf.append(CI.recv_buf.data(), protocol::DH_key_size);
		CI.recv_buf.erase(0, protocol::DH_key_size);
		return true;
	}
	return false;
}
//END key_exchange_rB

//BEGIN request_slot
message::request_slot::request_slot(
	boost::function<bool (boost::shared_ptr<base>)> func_in)
{
	func = func_in;
}

message::request_slot::request_slot(const std::string & hash)
{
	buf.append(protocol::request_slot).append(convert::hex_to_bin(hash));
}

bool message::request_slot::expects(network::connection_info & CI)
{
	return CI.recv_buf[0] == protocol::request_slot;
}

bool message::request_slot::recv(network::connection_info & CI)
{
	assert(expects(CI));
	if(CI.recv_buf.size() >= protocol::request_slot_size){
		buf.append(CI.recv_buf.data(), protocol::request_slot_size);
		CI.recv_buf.erase(0, protocol::request_slot_size);
		return true;
	}
	return false;
}
//END request_slot

//BEGIN slot
message::slot::slot(
	boost::function<bool (boost::shared_ptr<base>)> func_in,
	const std::string & hash_in
):
	hash(hash_in),
	checked(false)
{
LOGGER << hash;
	func = func_in;
}

message::slot::slot(const unsigned char slot_num, unsigned char status,
	const boost::uint64_t file_size, const std::string & root_hash)
{
	buf.append(protocol::slot)
		.append(slot_num)
		.append(status)
		.append(convert::encode(file_size))
		.append(convert::hex_to_bin(root_hash));
}

bool message::slot::expects(network::connection_info & CI)
{
	return CI.recv_buf[0] == protocol::slot;
}

bool message::slot::recv(network::connection_info & CI)
{
	assert(expects(CI));
	if(CI.recv_buf.size() < protocol::slot_size(0, 0)){
		//we do not have the minimum size slot message
		return false;
	}
	if(!checked){
		SHA1 SHA;
		SHA.init();
		SHA.load(reinterpret_cast<char *>(CI.recv_buf.data()) + 3,
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
		buf.append(CI.recv_buf.data(), protocol::slot_size(0, 0));
		CI.recv_buf.erase(0, protocol::slot_size(0, 0));
		return true;
	}else{
		LOGGER << "stub: need to process slot message where k != 0";
		exit(1);
	}
	return true;
}
//END slot
