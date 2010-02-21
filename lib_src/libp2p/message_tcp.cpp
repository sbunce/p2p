#include "message_tcp.hpp"

//BEGIN base
bool message_tcp::base::encrypt()
{
	return true;
}
//END base

//BEGIN block
message_tcp::block::block(
	handler func_in,
	const unsigned block_size_in,
	boost::shared_ptr<network::speed_calculator> Download_Speed
):
	func(func_in),
	block_size(block_size_in),
	bytes_seen(1)
{
	Speed_Calculator = Download_Speed;
}

message_tcp::block::block(network::buffer & block,
	boost::shared_ptr<network::speed_calculator> Upload_Speed)
{
	buf.append(protocol::block).append(block);
	Speed_Calculator = Upload_Speed;
}

bool message_tcp::block::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol::block;
}

message_tcp::status message_tcp::block::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= protocol::block_size(block_size)){
		Speed_Calculator->add(block_size - bytes_seen);
		buf.append(recv_buf.data() + 1, protocol::block_size(block_size) - 1);
		recv_buf.erase(0, protocol::block_size(block_size));
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	Speed_Calculator->add(recv_buf.size() - bytes_seen);
	bytes_seen = recv_buf.size();
	return incomplete;
}
//END block

//BEGIN close_slot
message_tcp::close_slot::close_slot(
	handler func_in
):
	func(func_in)
{

}

message_tcp::close_slot::close_slot(const unsigned char slot_num)
{
	buf.append(protocol::close_slot).append(slot_num);
}

bool message_tcp::close_slot::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol::close_slot;
}

message_tcp::status message_tcp::close_slot::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= protocol::close_slot_size){
		if(func(recv_buf[1])){
			return complete;
		}else{
			return blacklist;
		}
		recv_buf.erase(0, protocol::close_slot_size);
	}
	return incomplete;
}
//END close_slot

//BEGIN composite
void message_tcp::composite::add(boost::shared_ptr<base> M)
{
	assert(M);
	possible_response.push_back(M);
}

bool message_tcp::composite::expect(network::buffer & recv_buf)
{
	for(std::vector<boost::shared_ptr<base> >::iterator
		iter_cur = possible_response.begin(), iter_end = possible_response.end();
		iter_cur != iter_end; ++iter_cur)
	{
		status Status = (*iter_cur)->recv(recv_buf);
		if((*iter_cur)->expect(recv_buf)){
			return true;
		}
	}
	return false;
}

message_tcp::status message_tcp::composite::recv(network::buffer & recv_buf)
{
	for(std::vector<boost::shared_ptr<base> >::iterator
		iter_cur = possible_response.begin(), iter_end = possible_response.end();
		iter_cur != iter_end; ++iter_cur)
	{
		status Status = (*iter_cur)->recv(recv_buf);
		if(Status != not_expected){
			return Status;
		}
	}
	return not_expected;
}
//END composite

//BEGIN error
message_tcp::error::error(
	handler func_in
):
	func(func_in)
{

}

message_tcp::error::error()
{
	buf.append(protocol::error);
}

bool message_tcp::error::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol::error;
}

message_tcp::status message_tcp::error::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf[0] == protocol::error){
		buf.append(recv_buf.data(), protocol::error_size);
		recv_buf.erase(0, protocol::error_size);
		if(func()){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END error

//BEGIN have_file_block
message_tcp::have_file_block::have_file_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t file_block_count_in
):
	func(func_in),
	slot_num(slot_num_in),
	file_block_count(file_block_count_in)
{

}

message_tcp::have_file_block::have_file_block(
	const unsigned char slot_num_in,
	const boost::uint64_t block_num,
	const boost::uint64_t file_block_count_in
):
	slot_num(slot_num_in),
	file_block_count(file_block_count_in)
{
	buf.append(protocol::have_file_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, file_block_count));
}

bool message_tcp::have_file_block::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf[0] != protocol::have_file_block){
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

message_tcp::status message_tcp::have_file_block::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	unsigned expected_size = protocol::have_file_block_size(
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
//END have_file_block

//BEGIN have_hash_tree_block
message_tcp::have_hash_tree_block::have_hash_tree_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t tree_block_count_in
):
	func(func_in),
	slot_num(slot_num_in),
	tree_block_count(tree_block_count_in)
{

}

message_tcp::have_hash_tree_block::have_hash_tree_block(
	const unsigned char slot_num_in,
	const boost::uint64_t block_num,
	const boost::uint64_t tree_block_count_in
):
	slot_num(slot_num_in),
	tree_block_count(tree_block_count_in)
{
	buf.append(protocol::have_hash_tree_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, tree_block_count));
}

bool message_tcp::have_hash_tree_block::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf[0] != protocol::have_hash_tree_block){
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

message_tcp::status message_tcp::have_hash_tree_block::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	unsigned expected_size = protocol::have_hash_tree_block_size(
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
//END have_hash_tree_block

//BEGIN initial
message_tcp::initial::initial(
	handler func_in
):
	func(func_in)
{

}

message_tcp::initial::initial(const std::string ID)
{
	assert(ID.size() == SHA1::hex_size);
	buf = convert::hex_to_bin(ID);
}

bool message_tcp::initial::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//initial message random, no way to verify we expect
	return true;
}

message_tcp::status message_tcp::initial::recv(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= SHA1::bin_size){
		buf.append(recv_buf.data(), SHA1::bin_size);
		recv_buf.erase(0, SHA1::bin_size);
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END initial

//BEGIN key_exchange_p_rA
message_tcp::key_exchange_p_rA::key_exchange_p_rA(
	handler func_in
):
	func(func_in)
{

}

message_tcp::key_exchange_p_rA::key_exchange_p_rA(encryption & Encryption)
{
	buf = Encryption.send_p_rA();
}

bool message_tcp::key_exchange_p_rA::encrypt()
{
	return false;
}

bool message_tcp::key_exchange_p_rA::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//no way to verify we expect
	return true;
}

message_tcp::status message_tcp::key_exchange_p_rA::recv(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= protocol::DH_key_size * 2){
		buf.append(recv_buf.data(), protocol::DH_key_size * 2);
		recv_buf.erase(0, protocol::DH_key_size * 2);
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END key_exchange_p_rA

//BEGIN key_exchange_rB
message_tcp::key_exchange_rB::key_exchange_rB(
	handler func_in
):
	func(func_in)
{

}

message_tcp::key_exchange_rB::key_exchange_rB(encryption & Encryption)
{
	buf = Encryption.send_rB();
}

bool message_tcp::key_exchange_rB::encrypt()
{
	return false;
}

bool message_tcp::key_exchange_rB::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	//no way to verify we expect
	return true;
}

message_tcp::status message_tcp::key_exchange_rB::recv(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() >= protocol::DH_key_size){
		buf.append(recv_buf.data(), protocol::DH_key_size);
		recv_buf.erase(0, protocol::DH_key_size);
		if(func(buf)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END key_exchange_rB

//BEGIN request_hash_tree_block
message_tcp::request_hash_tree_block::request_hash_tree_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t tree_block_count
):
	func(func_in),
	slot_num(slot_num_in),
	VLI_size(convert::VLI_size(tree_block_count))
{

}


message_tcp::request_hash_tree_block::request_hash_tree_block(
	const unsigned char slot_num,
	const boost::uint64_t block_num,
	const boost::uint64_t tree_block_count
)
{
	buf.append(protocol::request_hash_tree_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, tree_block_count));
}

bool message_tcp::request_hash_tree_block::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() == 1){
		return recv_buf[0] == protocol::request_hash_tree_block;
	}else{
		return recv_buf[0] == protocol::request_hash_tree_block && recv_buf[1] == slot_num;
	}
}

message_tcp::status message_tcp::request_hash_tree_block::recv(network::buffer & recv_buf)
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
//END request_hash_tree_block

//BEGIN request_file_block
message_tcp::request_file_block::request_file_block(
	handler func_in,
	const unsigned char slot_num_in,
	const boost::uint64_t tree_block_count
):
	func(func_in),
	slot_num(slot_num_in),
	VLI_size(convert::VLI_size(tree_block_count))
{

}

message_tcp::request_file_block::request_file_block(
	const unsigned char slot_num,
	const boost::uint64_t block_num,
	const boost::uint64_t file_block_count)
{
	buf.append(protocol::request_file_block)
		.append(slot_num)
		.append(convert::int_to_bin_VLI(block_num, file_block_count));
}

bool message_tcp::request_file_block::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	if(recv_buf.size() == 1){
		return recv_buf[0] == protocol::request_file_block;
	}else{
		return recv_buf[0] == protocol::request_file_block && recv_buf[1] == slot_num;
	}
}

message_tcp::status message_tcp::request_file_block::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= 2 + VLI_size){
		unsigned char slot_num = recv_buf[1];
		boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
			reinterpret_cast<char *>(recv_buf.data()) + 2, VLI_size));
		recv_buf.erase(0, protocol::request_file_block_size(VLI_size));
		if(func(slot_num, block_num)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END request_file_block

//BEGIN request_slot
message_tcp::request_slot::request_slot(
	handler func_in
):
	func(func_in)
{

}

message_tcp::request_slot::request_slot(const std::string & hash)
{
	buf.append(protocol::request_slot).append(convert::hex_to_bin(hash));
}

bool message_tcp::request_slot::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol::request_slot;
}

message_tcp::status message_tcp::request_slot::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() >= protocol::request_slot_size){
		std::string hash = convert::bin_to_hex(std::string(
			reinterpret_cast<const char *>(recv_buf.data()) + 1, SHA1::bin_size));
		recv_buf.erase(0, protocol::request_slot_size);
		if(func(hash)){
			return complete;
		}else{
			return blacklist;
		}
	}
	return incomplete;
}
//END request_slot

//BEGIN slot
message_tcp::slot::slot(
	handler func_in,
	const std::string & hash_in
):
	func(func_in),
	hash(hash_in),
	checked(false)
{

}

message_tcp::slot::slot(const unsigned char slot_num,
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
	buf.append(protocol::slot)
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

bool message_tcp::slot::expect(network::buffer & recv_buf)
{
	assert(!recv_buf.empty());
	return recv_buf[0] == protocol::slot;
}

message_tcp::status message_tcp::slot::recv(network::buffer & recv_buf)
{
	if(!expect(recv_buf)){
		return not_expected;
	}
	if(recv_buf.size() < protocol::slot_size(0, 0)){
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
		recv_buf.erase(0, protocol::slot_size(0, 0));
	}else if(recv_buf[2] == 1){
		//file bit_field
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count, 1);
		if(recv_buf.size() < protocol::slot_size(0, file_BF_size)){
			return incomplete;
		}
		file_BF.set_buf(recv_buf.data() + 31, file_BF_size, file_block_count, 1);
		recv_buf.erase(0, protocol::slot_size(0, file_BF_size));
	}else if(recv_buf[2] == 2){
		//tree bit_field
		boost::uint64_t tree_block_count = hash_tree::calc_tree_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count, 1);
		if(recv_buf.size() < protocol::slot_size(tree_BF_size, 0)){
			return incomplete;
		}
		tree_BF.set_buf(recv_buf.data() + 31, tree_BF_size, tree_block_count, 1);
		recv_buf.erase(0, protocol::slot_size(tree_BF_size, 0));
	}else if(recv_buf[2] == 3){
		//tree and file bit_field
		boost::uint64_t tree_block_count = hash_tree::calc_tree_block_count(file_size);
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count, 1);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count, 1);
		if(recv_buf.size() < protocol::slot_size(tree_BF_size, file_BF_size)){
			return incomplete;
		}
		tree_BF.set_buf(recv_buf.data() + 31, tree_BF_size, tree_block_count, 1);
		file_BF.set_buf(recv_buf.data() + 31 + tree_BF_size, file_BF_size, file_block_count, 1);
		recv_buf.erase(0, protocol::slot_size(tree_BF_size, file_BF_size));
	}else{
		LOGGER << "invalid status byte";
		return blacklist;
	}
	if(func(slot_num, file_size, root_hash, tree_BF, file_BF)){
		return complete;
	}else{
		return blacklist;
	}
}
//END slot
