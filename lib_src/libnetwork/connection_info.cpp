#include <network/network.hpp>

network::connection_info::connection_info(
	const int connection_ID_in,
	const std::string & host_in,
	const std::string & IP_in,
	const std::string & port_in,
	const direction_t direction_in
):
	connection_ID(connection_ID_in),
	host(host_in),
	IP(IP_in),
	port(port_in),
	direction(direction_in),
	latest_recv(0),
	latest_send(0),
	send_buf_size(0)
{

}
