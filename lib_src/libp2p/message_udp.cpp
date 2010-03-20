#include "message_udp.hpp"

//BEGIN recv::ping
message_udp::recv::ping::ping(
	handler func_in
):
	func(func_in)
{

}

bool message_udp::recv::ping::expect(const network::buffer & recv_buf)
{
	return recv_buf.size() == protocol_udp::ping_size
		&& recv_buf[0] == protocol_udp::ping;
}

bool message_udp::recv::ping::recv(const network::buffer & recv_buf,
	const network::endpoint & endpoint)
{
	if(!expect(recv_buf)){
		return false;
	}
	network::buffer random(recv_buf.data()+1, 8);
	func(endpoint, random);
	return true;
}
//END recv::ping

//BEGIN recv::pong
message_udp::recv::pong::pong(
	handler func_in,
	const network::buffer & random_in
):
	func(func_in),
	random(random_in)
{

}

bool message_udp::recv::pong::expect(const network::buffer & recv_buf)
{
	if(recv_buf.size() != protocol_udp::pong_size){
		return false;
	}
	if(recv_buf[0] != protocol_udp::pong){
		return false;
	}
	return std::memcmp(recv_buf.data()+1, random.data(), 8) == 0;
}

bool message_udp::recv::pong::recv(const network::buffer & recv_buf,
	const network::endpoint & endpoint)
{
	if(!expect(recv_buf)){
		return false;
	}
	std::string ID(convert::bin_to_hex(std::string(reinterpret_cast<const char *>(
		recv_buf.data())+9, 20)));
	func(endpoint, ID);
	return true;
}
//END recv::pong

//BEGIN send::ping
message_udp::send::ping::ping(const network::buffer & random)
{
	assert(random.size() == 8);
	buf.append(protocol_udp::ping).append(random);
}
//END send::ping

//BEGIN send::pong
message_udp::send::pong::pong(const network::buffer & random,
	const std::string & ID)
{
	assert(random.size() == 8);
	assert(ID.size() == SHA1::hex_size);
	buf.append(protocol_udp::pong).append(random).append(convert::hex_to_bin(ID));
}
//END send::ping
