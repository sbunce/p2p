#include "message_tcp.hpp"

//BEGIN send::base
bool message_tcp::send::base::encrypt()
{
	return true;
}
//END send::base

//BEGIN recv::block
message_tcp::recv::block::block(
	handler func_in,
	const unsigned block_size_in,
	speed_composite Download_Speed_in
):
	func(func_in),
	block_size(block_size_in),
	bytes_seen(1)
{
	Download_Speed = Download_Speed_in;
}

bool message_tcp::recv::block::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol_tcp::block;
}

message_tcp::recv::status message_tcp::recv::block::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= protocol_tcp::block_size(block_size)){
		net::buffer buf;
		Download_Speed.add(block_size - bytes_seen);
		buf.append(recv_buf.data() + 1, protocol_tcp::block_size(block_size) - 1);
		recv_buf.erase(0, protocol_tcp::block_size(block_size));
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	Download_Speed.add(recv_buf.size() - bytes_seen);
	bytes_seen = recv_buf.size();
	return incomplete;
}
//END recv::block

//BEGIN recv::close_slot
message_tcp::recv::close_slot::close_slot(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::close_slot::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol_tcp::close_slot;
}

message_tcp::recv::status message_tcp::recv::close_slot::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= protocol_tcp::close_slot_size){
		unsigned char slot_num = recv_buf[1];
		recv_buf.erase(0, protocol_tcp::close_slot_size);
		if(func(slot_num)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::close_slot

//BEGIN recv::composite
void message_tcp::recv::composite::add(boost::shared_ptr<base> M)
{
	assert(M);
	possible_response.push_back(M);
}

bool message_tcp::recv::composite::expect(const net::buffer & recv_buf)
{
	for(std::vector<boost::shared_ptr<base> >::iterator
		it_cur = possible_response.begin(), it_end = possible_response.end();
		it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->expect(recv_buf)){
			return true;
		}
	}
	return false;
}

message_tcp::recv::status message_tcp::recv::composite::recv(net::buffer & recv_buf)
{
	for(std::vector<boost::shared_ptr<base> >::iterator
		it_cur = possible_response.begin(), it_end = possible_response.end();
		it_cur != it_end; ++it_cur)
	{
		status Status = (*it_cur)->recv(recv_buf);
		if(Status != not_expected){
			return Status;
		}
	}
	return not_expected;
}
//END recv::composite

//BEGIN recv::error
message_tcp::recv::error::error(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::error::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol_tcp::error;
}

message_tcp::recv::status message_tcp::recv::error::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf[0] == protocol_tcp::error){
		net::buffer buf;
		buf.append(recv_buf.data(), protocol_tcp::error_size);
		recv_buf.erase(0, protocol_tcp::error_size);
		if(func()){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::error

//BEGIN recv::have_file_block
message_tcp::recv::have_file_block::have_file_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t file_block_count_in
):
	func(func_in),
	slot_num(slot_num_in),
	file_block_count(file_block_count_in)
{

}

bool message_tcp::recv::have_file_block::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf[0] != protocol_tcp::have_file_block){
		return false;
	}
	if(recv_buf.size() == 1){
		//don't have slot_num, assume we expect
		return true;
	}else{
		//recv_buf.size() >= 2
		return recv_buf[1] == slot_num;
	}
}

message_tcp::recv::status message_tcp::recv::have_file_block::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	unsigned expected_size = protocol_tcp::have_file_block_size(
		convert::VLI_size(file_block_count));
	if(recv_buf.size() >= expected_size){
		unsigned char slot_num = recv_buf[1];
		boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
		reinterpret_cast<char *>(recv_buf.data()) + 2, expected_size - 2));
		recv_buf.erase(0, expected_size);
		if(func(slot_num, block_num)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::have_file_block

//BEGIN recv::have_hash_tree_block
message_tcp::recv::have_hash_tree_block::have_hash_tree_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t tree_block_count_in
):
	func(func_in),
	slot_num(slot_num_in),
	tree_block_count(tree_block_count_in)
{

}

bool message_tcp::recv::have_hash_tree_block::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf[0] != protocol_tcp::have_hash_tree_block){
		return false;
	}
	if(recv_buf.size() == 1){
		//don't have slot_num, assume we expect
		return true;
	}else{
		//recv_buf.size() >= 2
		return recv_buf[1] == slot_num;
	}
}

message_tcp::recv::status message_tcp::recv::have_hash_tree_block::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	unsigned expected_size = protocol_tcp::have_hash_tree_block_size(
		convert::VLI_size(tree_block_count));
	if(recv_buf.size() >= expected_size){
		unsigned char slot_num = recv_buf[1];
		boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
		reinterpret_cast<char *>(recv_buf.data()) + 2, expected_size - 2));
		recv_buf.erase(0, expected_size);
		if(func(slot_num, block_num)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::have_hash_tree_block

//BEGIN recv::initial_ID
message_tcp::recv::initial_ID::initial_ID(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::initial_ID::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//initial message random, no way to verify we expect
	return true;
}

message_tcp::recv::status message_tcp::recv::initial_ID::recv(net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= protocol_tcp::initial_ID_size){
		std::string ID = convert::bin_to_hex(std::string(
			reinterpret_cast<const char *>(recv_buf.data()), SHA1::bin_size));
		recv_buf.erase(0, protocol_tcp::initial_ID_size);
		if(func(ID)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::initial_ID

//BEGIN recv::initial_port
message_tcp::recv::initial_port::initial_port(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::initial_port::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//initial message random, no way to verify we expect
	return true;
}

message_tcp::recv::status message_tcp::recv::initial_port::recv(net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= protocol_tcp::initial_port_size){
		std::string port = boost::lexical_cast<std::string>(
			convert::bin_to_int<boost::uint16_t>(std::string(
			reinterpret_cast<const char *>(recv_buf.data()), 2)));
		recv_buf.erase(0, protocol_tcp::initial_port_size);
		if(func(port)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::initial_ID

//BEGIN recv::key_exchange_p_rA
message_tcp::recv::key_exchange_p_rA::key_exchange_p_rA(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::key_exchange_p_rA::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//no way to verify we expect
	return true;
}

message_tcp::recv::status message_tcp::recv::key_exchange_p_rA::recv(net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= protocol_tcp::DH_key_size * 2){
		net::buffer buf;
		buf.append(recv_buf.data(), protocol_tcp::DH_key_size * 2);
		recv_buf.erase(0, protocol_tcp::DH_key_size * 2);
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::key_exchange_p_rA

//BEGIN recv::key_exchange_rB
message_tcp::recv::key_exchange_rB::key_exchange_rB(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::key_exchange_rB::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//no way to verify we expect
	return true;
}

message_tcp::recv::status message_tcp::recv::key_exchange_rB::recv(net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= protocol_tcp::DH_key_size){
		net::buffer buf;
		buf.append(recv_buf.data(), protocol_tcp::DH_key_size);
		recv_buf.erase(0, protocol_tcp::DH_key_size);
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::key_exchange_rB

//BEGIN recv::request_file_block
message_tcp::recv::request_file_block::request_file_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t tree_block_count
):
	func(func_in),
	slot_num(slot_num_in),
	VLI_size(convert::VLI_size(tree_block_count))
{

}

bool message_tcp::recv::request_file_block::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() == 1){
		return recv_buf[0] == protocol_tcp::request_file_block;
	}else{
		return recv_buf[0] == protocol_tcp::request_file_block
			&& recv_buf[1] == slot_num;
	}
}

message_tcp::recv::status message_tcp::recv::request_file_block::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= 2 + VLI_size){
		unsigned char slot_num = recv_buf[1];
		boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
			reinterpret_cast<char *>(recv_buf.data()) + 2, VLI_size));
		recv_buf.erase(0, protocol_tcp::request_file_block_size(VLI_size));
		if(func(slot_num, block_num)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::request_file_block

//BEGIN recv::request_hash_tree_block
message_tcp::recv::request_hash_tree_block::request_hash_tree_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t tree_block_count
):
	func(func_in),
	slot_num(slot_num_in),
	VLI_size(convert::VLI_size(tree_block_count))
{

}

bool message_tcp::recv::request_hash_tree_block::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() == 1){
		return recv_buf[0] == protocol_tcp::request_hash_tree_block;
	}else{
		return recv_buf[0] == protocol_tcp::request_hash_tree_block
			&& recv_buf[1] == slot_num;
	}
}

message_tcp::recv::status message_tcp::recv::request_hash_tree_block::recv(
	net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= 2 + VLI_size){
		unsigned char slot_num = recv_buf[1];
		boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
			reinterpret_cast<char *>(recv_buf.data()) + 2, VLI_size));
		recv_buf.erase(0, 2 + VLI_size);
		if(func(slot_num, block_num)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::request_hash_tree_block

//BEGIN recv::request_slot
message_tcp::recv::request_slot::request_slot(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::request_slot::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol_tcp::request_slot;
}

message_tcp::recv::status message_tcp::recv::request_slot::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= protocol_tcp::request_slot_size){
		std::string hash = convert::bin_to_hex(std::string(
			reinterpret_cast<const char *>(recv_buf.data()) + 1, SHA1::bin_size));
		recv_buf.erase(0, protocol_tcp::request_slot_size);
		if(func(hash)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END recv::request_slot

//BEGIN recv::slot
message_tcp::recv::slot::slot(
	handler func_in,
	const std::string & hash_in
):
	func(func_in),
	hash(hash_in),
	checked(false)
{

}

bool message_tcp::recv::slot::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol_tcp::slot;
}

message_tcp::recv::status message_tcp::recv::slot::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() < protocol_tcp::slot_size(0, 0)){
		//do not have the minimum size slot message
		return incomplete;
	}
	if(!checked){
		SHA1 SHA;
		SHA.init();
		SHA.load(reinterpret_cast<char *>(recv_buf.data()) + 3,
			8 + SHA1::bin_size);
		SHA.end();
		if(SHA.hex() != hash){
			return blacklist;
		}
	}
	unsigned char slot_num = recv_buf[1];
	boost::uint64_t file_size = convert::bin_to_int<boost::uint64_t>(
		std::string(reinterpret_cast<char *>(recv_buf.data()) + 3, 8));
	std::string root_hash = convert::bin_to_hex(std::string(
		reinterpret_cast<char *>(recv_buf.data()+11), SHA1::bin_size));
	bit_field tree_BF, file_BF;
	if(recv_buf[2] == 0){
		//no bit_field
		recv_buf.erase(0, protocol_tcp::slot_size(0, 0));
	}else if(recv_buf[2] == 1){
		//file bit_field
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count);
		if(recv_buf.size() < protocol_tcp::slot_size(0, file_BF_size)){
			return incomplete;
		}
		file_BF.set_buf(recv_buf.data() + 31, file_BF_size, file_block_count);
		recv_buf.erase(0, protocol_tcp::slot_size(0, file_BF_size));
	}else if(recv_buf[2] == 2){
		//tree bit_field
		boost::uint64_t tree_block_count = tree_info::calc_tree_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count);
		if(recv_buf.size() < protocol_tcp::slot_size(tree_BF_size, 0)){
			return incomplete;
		}
		tree_BF.set_buf(recv_buf.data() + 31, tree_BF_size, tree_block_count);
		recv_buf.erase(0, protocol_tcp::slot_size(tree_BF_size, 0));
	}else if(recv_buf[2] == 3){
		//tree and file bit_field
		boost::uint64_t tree_block_count = tree_info::calc_tree_block_count(file_size);
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count);
		if(recv_buf.size() < protocol_tcp::slot_size(tree_BF_size, file_BF_size)){
			return incomplete;
		}
		tree_BF.set_buf(recv_buf.data() + 31, tree_BF_size, tree_block_count);
		file_BF.set_buf(recv_buf.data() + 31 + tree_BF_size, file_BF_size, file_block_count);
		recv_buf.erase(0, protocol_tcp::slot_size(tree_BF_size, file_BF_size));
	}else{
		LOG << "invalid status byte";
		return blacklist;
	}
	if(func(slot_num, file_size, root_hash, tree_BF, file_BF)){
		return complete;
	}else{
		return blacklist;
	}
}
//END recv::slot

//BEGIN recv::peer
message_tcp::recv::peer::peer(
	handler func_in
):
	func(func_in)
{

}

bool message_tcp::recv::peer::expect(const net::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol_tcp::peer_4
		|| recv_buf[0] == protocol_tcp::peer_6;
}

message_tcp::recv::status message_tcp::recv::peer::recv(net::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf[0] == protocol_tcp::peer_4){
		if(recv_buf.size() < protocol_tcp::peer_4_size){
			return incomplete;
		}
		unsigned char slot_num = recv_buf[1];
		boost::optional<net::endpoint> ep = net::bin_to_endpoint(
			std::string(reinterpret_cast<const char *>(recv_buf.data())+2, 4),
			std::string(reinterpret_cast<const char *>(recv_buf.data())+6, 2));
		if(ep){
			func(slot_num, *ep);
		}
		recv_buf.erase(0, protocol_tcp::peer_4_size);
		return complete;
	}else{
		if(recv_buf.size() < protocol_tcp::peer_6_size){
			return incomplete;
		}
		unsigned char slot_num = recv_buf[1];
		boost::optional<net::endpoint> ep = net::bin_to_endpoint(
			std::string(reinterpret_cast<const char *>(recv_buf.data())+2, 16),
			std::string(reinterpret_cast<const char *>(recv_buf.data())+18, 2));
		if(ep){
			func(slot_num, *ep);
		}
		recv_buf.erase(0, protocol_tcp::peer_6_size);
		return complete;
	}
}
//END recv::peer

//BEGIN send::block
message_tcp::send::block::block(const net::buffer & block,
	speed_composite Upload_Speed_in)
{
	buf.append(protocol_tcp::block).append(block);
	Upload_Speed = Upload_Speed_in;
}
//END send::block

//BEGIN send::close_slot
message_tcp::send::close_slot::close_slot(const unsigned char slot_num)
{
	buf.append(protocol_tcp::close_slot).append(slot_num);
}
//END send::close_slot

//BEGIN send::error
message_tcp::send::error::error()
{
	buf.append(protocol_tcp::error);
}
//END send::error

//BEGIN send::have_file_block
message_tcp::send::have_file_block::have_file_block(
	const unsigned char slot_num,
	const boost::uint64_t block_num,
	const boost::uint64_t file_block_count
)
{
	buf.append(protocol_tcp::have_file_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, file_block_count));
}
//END send::have_file_block

//BEGIN send::have_hash_tree_block
message_tcp::send::have_hash_tree_block::have_hash_tree_block(
	const unsigned char slot_num,
	const boost::uint64_t block_num,
	const boost::uint64_t tree_block_count
)
{
	buf.append(protocol_tcp::have_hash_tree_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, tree_block_count));
}
//END send::have_hash_tree_block

//BEGIN send::initial_ID
message_tcp::send::initial_ID::initial_ID(const std::string & ID)
{
	assert(ID.size() == SHA1::hex_size);
	buf.append(convert::hex_to_bin(ID));
}
//END send::initial_ID

//BEGIN send::initial_port
message_tcp::send::initial_port::initial_port(const std::string & port)
{
	try{
		buf.append(convert::int_to_bin(boost::lexical_cast<boost::uint16_t>(port)));
	}catch(const std::exception & e){
		LOG << e.what();
		exit(1);
	}
}
//END send::initial_port

//BEGIN send::key_exchange_p_rA
message_tcp::send::key_exchange_p_rA::key_exchange_p_rA(const net::buffer & buf_in)
{
	buf = buf_in;
}

bool message_tcp::send::key_exchange_p_rA::encrypt()
{
	return false;
}
//END send::key_exchange_p_rA

//BEGIN send::key_exchange_rB
message_tcp::send::key_exchange_rB::key_exchange_rB(const net::buffer & buf_in)
{
	buf = buf_in;
}

bool message_tcp::send::key_exchange_rB::encrypt()
{
	return false;
}
//END send::key_exchange_rB

//BEGIN send::request_file_block
message_tcp::send::request_file_block::request_file_block(
	const unsigned char slot_num,
	const boost::uint64_t block_num,
	const boost::uint64_t file_block_count)
{
	buf.append(protocol_tcp::request_file_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, file_block_count));
}
//END send::request_file_block

//BEGIN send::request_hash_tree_block
message_tcp::send::request_hash_tree_block::request_hash_tree_block(
	const unsigned char slot_num,
	const boost::uint64_t block_num,
	const boost::uint64_t tree_block_count
)
{
	buf.append(protocol_tcp::request_hash_tree_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, tree_block_count));
}
//END send::request_hash_tree_block

//BEGIN send::request_slot
message_tcp::send::request_slot::request_slot(const std::string & hash)
{
	assert(hash.size() == SHA1::hex_size);
	buf.append(protocol_tcp::request_slot).append(convert::hex_to_bin(hash));
}
//END send::request_slot

//BEGIN send::slot
message_tcp::send::slot::slot(const unsigned char slot_num,
	const boost::uint64_t file_size, const std::string & root_hash,
	const bit_field & tree_BF, const bit_field & file_BF)
{
	unsigned char status;
	if(tree_BF.empty() && file_BF.empty()){
		status = 0;
	}else if(tree_BF.empty() && !file_BF.empty()){
		status = 1;
	}else if(!tree_BF.empty() && file_BF.empty()){
		status = 2;
	}else if(!tree_BF.empty() && !file_BF.empty()){
		status = 3;
	}
	buf.append(protocol_tcp::slot)
		.append(slot_num)
		.append(status)
		.append(convert::int_to_bin(file_size))
		.append(convert::hex_to_bin(root_hash));
	if(!tree_BF.empty()){
		buf.append(tree_BF.get_buf());
	}
	if(!file_BF.empty()){
		buf.append(file_BF.get_buf());
	}
}
//END send::slot

//BEGIN send::peer
message_tcp::send::peer::peer(const unsigned char slot_num,
	const net::endpoint & ep)
{
	if(ep.version() == net::IPv4){
		buf.append(protocol_tcp::peer_4);
	}else{
		buf.append(protocol_tcp::peer_6);
	}
	buf.append(slot_num)
		.append(ep.IP_bin())
		.append(ep.port_bin());
}
//END send::peer
