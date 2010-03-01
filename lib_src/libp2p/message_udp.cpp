#include "message_udp.hpp"

//BEGIN recv::ping
message_udp::recv::ping::ping(
	handler func_in,
	const std::string & random_in
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
