//custom
#include "../message_tcp.hpp"

//include
#include <logger.hpp>
#include <net/net.hpp>
#include <unit_test.hpp>

int fail(0);

void append_garbage(net::buffer & block)
{
	block.append("123");
}


const net::buffer test_block("ABC");
bool block_call_back(const net::buffer & block)
{
	if(block != test_block){
		LOG; ++fail;
	}
	return true;
}

const unsigned char test_slot_num(0);
bool close_slot_call_back(const unsigned char slot_num)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	return true;
}

bool error_call_back()
{
	//nothing to check here
	return true;
}

const boost::uint64_t test_block_num(42);
const boost::uint64_t test_block_count(84);
bool have_call_back(const unsigned char slot_num,
	const boost::uint64_t block_num)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(block_num != test_block_num){
		LOG; ++fail;
	}
	return true;
}

const std::string test_ID("0123456789012345678901234567890123456789");
bool initial_ID_call_back(const std::string & remote_ID)
{
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
	return true;
}

const std::string test_port("1234");
bool initial_port_call_back(const std::string & port)
{
	if(port != test_port){
		LOG; ++fail;
	}
	return true;
}

//note, p_rA doesn't contain actual prime
const net::buffer test_p_rA(random::urandom(protocol_tcp::DH_key_size * 2));
bool key_exchange_p_rA_call_back(const net::buffer & p_rA)
{
	if(p_rA != test_p_rA){
		LOG; ++fail;
	}
	return true;
}

const net::buffer test_rB(random::urandom(protocol_tcp::DH_key_size));
bool key_exchange_rB_call_back(const net::buffer & rB)
{
	if(rB != test_rB){
		LOG; ++fail;
	}
	return true;
}

bool request_call_back(const unsigned char slot_num,
	const boost::uint64_t block_num)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(block_num != test_block_num){
		LOG; ++fail;
	}
	return true;
}

const std::string test_hash_request_slot("0123456789012345678901234567890123456789");
bool request_slot_call_back(const std::string & hash)
{
	if(hash != test_hash_request_slot){
		LOG; ++fail;
	}
	return true;
}

const boost::uint64_t test_file_size(protocol_tcp::file_block_size * 128);
const std::string test_root_hash("0123456789012345678901234567890123456789");
std::string test_hash_slot;
const boost::uint64_t tree_block_count(hash_tree::calc_tree_block_count(test_file_size));
const boost::uint64_t file_block_count(file::calc_file_block_count(test_file_size));
bit_field test_tree_BF(tree_block_count);
bit_field test_tree_BF_empty;
bit_field test_file_BF(file_block_count);
bit_field test_file_BF_empty;

//no bit field
bool slot_00_call_back(const unsigned char slot_num,
	const boost::uint64_t file_size, const std::string & root_hash,
	bit_field & tree_BF, bit_field & file_BF)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(file_size != test_file_size){
		LOG; ++fail;
	}
	if(root_hash != test_root_hash){
		LOG; ++fail;
	}
	if(!tree_BF.empty()){
		LOG; ++fail;
	}
	if(!file_BF.empty()){
		LOG; ++fail;
	}
	return true;
}

//file bit_field
bool slot_01_call_back(const unsigned char slot_num,
	const boost::uint64_t file_size, const std::string & root_hash,
	bit_field & tree_BF, bit_field & file_BF)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(file_size != test_file_size){
		LOG; ++fail;
	}
	if(root_hash != test_root_hash){
		LOG; ++fail;
	}
	if(!tree_BF.empty()){
		LOG; ++fail;
	}
	if(file_BF != test_file_BF){
		LOG; ++fail;
	}
	return true;
}

//tree bit_field
bool slot_10_call_back(const unsigned char slot_num,
	const boost::uint64_t file_size, const std::string & root_hash,
	bit_field & tree_BF, bit_field & file_BF)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(file_size != test_file_size){
		LOG; ++fail;
	}
	if(root_hash != test_root_hash){
		LOG; ++fail;
	}
	if(tree_BF != test_tree_BF){
		LOG; ++fail;
	}
	if(!file_BF.empty()){
		LOG; ++fail;
	}
	return true;
}

//tree and file bit_field
bool slot_11_call_back(const unsigned char slot_num,
	const boost::uint64_t file_size, const std::string & root_hash,
	bit_field & tree_BF, bit_field & file_BF)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(file_size != test_file_size){
		LOG; ++fail;
	}
	if(root_hash != test_root_hash){
		LOG; ++fail;
	}
	if(tree_BF != test_tree_BF){
		LOG; ++fail;
	}
	if(file_BF != test_file_BF){
		LOG; ++fail;
	}
	return true;
}

boost::shared_ptr<net::endpoint> test_ep;
bool peer_call_back(const unsigned char slot_num, const net::endpoint & ep)
{
	if(slot_num != test_slot_num){
		LOG; ++fail;
	}
	if(ep != *test_ep){
		LOG; ++fail;
	}
	return true;
}

int main()
{
	unit_test::timeout();

	boost::shared_ptr<message_tcp::recv::base> M_recv;
	boost::shared_ptr<message_tcp::send::base> M_send;
	speed_composite SC;

	//block
	M_recv.reset(new message_tcp::recv::block(&block_call_back, test_block.size(), SC));
	M_send.reset(new message_tcp::send::block(test_block, SC));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//close_slot
	M_recv.reset(new message_tcp::recv::close_slot(&close_slot_call_back));
	M_send.reset(new message_tcp::send::close_slot(test_slot_num));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//composite holding block and close_slot
	boost::shared_ptr<message_tcp::recv::composite> M_composite(new message_tcp::recv::composite());
	M_recv.reset(new message_tcp::recv::block(&block_call_back, test_block.size(), SC));
	M_composite->add(M_recv);
	M_recv.reset(new message_tcp::recv::close_slot(&close_slot_call_back));
	M_composite->add(M_recv);
	M_send.reset(new message_tcp::send::block(test_block, SC));
	append_garbage(M_send->buf);
	if(M_composite->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}
	M_send.reset(new message_tcp::send::close_slot(test_slot_num));
	append_garbage(M_send->buf);
	if(M_composite->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//error
	M_recv.reset(new message_tcp::recv::error(&error_call_back));
	M_send.reset(new message_tcp::send::error());
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//have_file_block
	M_recv.reset(new message_tcp::recv::have_file_block(&have_call_back,
		test_slot_num, test_block_count));
	M_send.reset(new message_tcp::send::have_file_block(test_slot_num,
		test_block_num, test_block_count));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//have_hash_tree_block
	M_recv.reset(new message_tcp::recv::have_hash_tree_block(&have_call_back,
		test_slot_num, test_block_count));
	M_send.reset(new message_tcp::send::have_hash_tree_block(test_slot_num,
		test_block_num, test_block_count));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//initial_ID
	M_recv.reset(new message_tcp::recv::initial_ID(&initial_ID_call_back));
	M_send.reset(new message_tcp::send::initial_ID(test_ID));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//initial_port
	M_recv.reset(new message_tcp::recv::initial_port(&initial_port_call_back));
	M_send.reset(new message_tcp::send::initial_port(test_port));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//key_exchange_p_rA
	M_recv.reset(new message_tcp::recv::key_exchange_p_rA(&key_exchange_p_rA_call_back));
	M_send.reset(new message_tcp::send::key_exchange_p_rA(test_p_rA));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//key_exchange_rB
	M_recv.reset(new message_tcp::recv::key_exchange_rB(&key_exchange_rB_call_back));
	M_send.reset(new message_tcp::send::key_exchange_rB(test_rB));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//request_file_block
	M_recv.reset(new message_tcp::recv::request_file_block(&request_call_back,
		test_slot_num, test_block_count));
	M_send.reset(new message_tcp::send::request_file_block(test_slot_num,
		test_block_num, test_block_count));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//request_hash_tree_block
	M_recv.reset(new message_tcp::recv::request_hash_tree_block(&request_call_back,
		test_slot_num, test_block_count));
	M_send.reset(new message_tcp::send::request_hash_tree_block(test_slot_num,
		test_block_num, test_block_count));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//request_slot
	M_recv.reset(new message_tcp::recv::request_slot(&request_slot_call_back));
	M_send.reset(new message_tcp::send::request_slot(test_hash_request_slot));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//slot
	//calculate hash
	char buf[8 + SHA1::bin_size];
	SHA1 SHA;
	std::memcpy(buf, convert::int_to_bin(test_file_size).data(), 8);
	std::memcpy(buf + 8, convert::hex_to_bin(test_root_hash).data(), SHA1::bin_size);
	SHA.init();
	SHA.load(buf, SHA1::bin_size + 8);
	SHA.end();
	test_hash_slot = SHA.hex();

	//no bit_field
	M_recv.reset(new message_tcp::recv::slot(&slot_00_call_back, test_hash_slot));
	M_send.reset(new message_tcp::send::slot(test_slot_num, test_file_size,
		test_root_hash, test_tree_BF_empty, test_file_BF_empty));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//file bit_field
	M_recv.reset(new message_tcp::recv::slot(&slot_01_call_back, test_hash_slot));
	M_send.reset(new message_tcp::send::slot(test_slot_num, test_file_size,
		test_root_hash, test_tree_BF_empty, test_file_BF));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//tree bit_field
	M_recv.reset(new message_tcp::recv::slot(&slot_10_call_back, test_hash_slot));
	M_send.reset(new message_tcp::send::slot(test_slot_num, test_file_size,
		test_root_hash, test_tree_BF, test_file_BF_empty));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//tree and file bit_field
	M_recv.reset(new message_tcp::recv::slot(&slot_11_call_back, test_hash_slot));
	M_send.reset(new message_tcp::send::slot(test_slot_num, test_file_size,
		test_root_hash, test_tree_BF, test_file_BF));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}

	//peer
	std::set<net::endpoint> E = net::get_endpoint("127.0.0.1", "0");
	assert(!E.empty());
	test_ep.reset(new net::endpoint(*E.begin()));
	M_recv.reset(new message_tcp::recv::peer(&peer_call_back));
	M_send.reset(new message_tcp::send::peer(test_slot_num, *test_ep));
	append_garbage(M_send->buf);
	if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
		LOG; ++fail;
	}
	//test IPv6 only if host supports it
	E = net::get_endpoint("::1", "0");
	if(!E.empty()){
		test_ep.reset(new net::endpoint(*E.begin()));
		M_recv.reset(new message_tcp::recv::peer(&peer_call_back));
		M_send.reset(new message_tcp::send::peer(test_slot_num, *test_ep));
		append_garbage(M_send->buf);
		if(M_recv->recv(M_send->buf) != message_tcp::recv::complete){
			LOG; ++fail;
		}
	}

	return fail;
}
