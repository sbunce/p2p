#include "message_udp.hpp"

//BEGIN recv::ping
message_udp::recv::ping::ping(
	handler func_in,
	const network::buffer & random_in
):
	func(func_in),
	random(random_in)
{

}

bool message_udp::recv::ping::expect(const network::buffer & recv_buf)
{

}

bool message_udp::recv::ping::recv(network::buffer & recv_buf)
{

}
//END recv::ping

//BEGIN send::ping
message_udp::send::ping::ping(const network::buffer & random,
	const std::string & ID)
{
	//buf.append(
}
//END send::ping
