#include "connection.hpp"

connection::connection(
	network::sock & S,
	prime_generator & Prime_Generator_in
):
	Prime_Generator(Prime_Generator_in),
	Encryption(Prime_Generator)
{

}

void connection::recv_call_back(network::sock & S)
{

}

void connection::send_call_back(network::sock & S)
{

}
